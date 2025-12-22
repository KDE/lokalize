/*
  This file is part of Lokalize

  SPDX-FileCopyrightText: 2007-2009 Nick Shaforostoff <shafff@ukr.net>
  SPDX-FileCopyrightText: 2018-2019 Simon Depiets <sdepiets@gmail.com>

  SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#include "cataloglistview.h"
#include "catalog.h"
#include "catalogmodel.h"
#include "headerviewmenu.h"
#include "prefs.h"
#include "project.h"

#include <KConfigGroup>
#include <KLocalizedString>

#include <QAction>
#include <QActionGroup>
#include <QHeaderView>
#include <QKeyEvent>
#include <QLineEdit>
#include <QMenu>
#include <QModelIndex>
#include <QShortcut>
#include <QTime>
#include <QToolButton>
#include <QTreeView>
#include <QVBoxLayout>

class CatalogTreeView : public QTreeView
{
public:
    explicit CatalogTreeView(QWidget *parent)
        : QTreeView(parent)
    {
    }
    ~CatalogTreeView() = default;

protected:
    void keyReleaseEvent(QKeyEvent *e) override
    {
        if (e->key() == Qt::Key_Return && currentIndex().isValid()) {
            Q_EMIT clicked(currentIndex());
            e->accept();
        } else {
            QTreeView::keyReleaseEvent(e);
        }
    }
};

CatalogView::CatalogView(QWidget *parent, Catalog *catalog)
    : QDockWidget(i18nc("@title:window aka Message Tree", "Translation Units"), parent)
    , m_browser(new CatalogTreeView(this))
    , m_lineEdit(new QLineEdit(this))
    , m_model(new CatalogTreeModel(this, catalog))
    , m_proxyModel(new CatalogTreeFilterModel(this))
{
    setObjectName(QStringLiteral("catalogTreeView"));

    QWidget *w = new QWidget(this);
    QVBoxLayout *mainLayout = new QVBoxLayout(w);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);
    QHBoxLayout *searchAndOptionsButtonLayout = new QHBoxLayout;
    searchAndOptionsButtonLayout->setContentsMargins(style()->pixelMetric(QStyle::PM_LayoutLeftMargin),
                                                     style()->pixelMetric(QStyle::PM_LayoutTopMargin),
                                                     style()->pixelMetric(QStyle::PM_LayoutRightMargin),
                                                     style()->pixelMetric(QStyle::PM_LayoutBottomMargin));
    searchAndOptionsButtonLayout->setSpacing(style()->pixelMetric(QStyle::PM_LayoutHorizontalSpacing));
    mainLayout->addLayout(searchAndOptionsButtonLayout);

    QFrame *horizontalLine = new QFrame();
    horizontalLine->setFrameShape(QFrame::HLine);
    mainLayout->addWidget(horizontalLine);

    m_lineEdit->setClearButtonEnabled(true);
    m_lineEdit->setPlaceholderText(i18n("Search"));
    m_lineEdit->setToolTip(i18nc("@info:tooltip", "Activated by Ctrl+L. Accepts regular expressions"));
    connect(m_lineEdit, &QLineEdit::textChanged, this, &CatalogView::setFilterRegExp, Qt::QueuedConnection);
    QShortcut *esc = new QShortcut(QKeySequence(Qt::Key_Escape), this, nullptr, nullptr, Qt::WidgetWithChildrenShortcut);
    connect(esc, &QShortcut::activated, this, &CatalogView::escaped);

    QToolButton *btn = new QToolButton(w);
    btn->setPopupMode(QToolButton::InstantPopup);
    btn->setText(i18n("Options"));
    btn->setMenu(new QMenu(this));
    m_filterOptionsMenu = btn->menu();
    connect(m_filterOptionsMenu, &QMenu::aboutToShow, this, &CatalogView::fillFilterOptionsMenu);
    connect(m_filterOptionsMenu, &QMenu::triggered, this, &CatalogView::filterOptionToggled);

    searchAndOptionsButtonLayout->addWidget(m_lineEdit);
    searchAndOptionsButtonLayout->addWidget(btn);
    mainLayout->addWidget(m_browser);

    setTabOrder(m_lineEdit, btn);
    setTabOrder(btn, m_browser);
    setFocusProxy(m_lineEdit);

    setWidget(w);

    connect(m_browser, &CatalogTreeView::clicked, this, &CatalogView::slotItemActivated);
    m_browser->setRootIsDecorated(false);
    m_browser->setAllColumnsShowFocus(true);
    m_browser->setAlternatingRowColors(true);
    m_browser->viewport()->setBackgroundRole(QPalette::Window);
#ifdef Q_OS_DARWIN
    QPalette p;
    p.setColor(QPalette::AlternateBase, p.color(QPalette::Window).darker(110));
    p.setColor(QPalette::Highlight, p.color(QPalette::Window).darker(150));
    m_browser->setPalette(p);
#endif

    // Tell the consumer (EditorTab) that it needs to check again whether the
    // current entry is the first/last filtered entry or not..
    // layoutChanged fires when the sort changes.
    connect(m_proxyModel, &CatalogTreeFilterModel::layoutChanged, this, &CatalogView::entryProxiedPositionChanged);
    // rowsInserted and rowsRemoved fire when the filtering changes.
    connect(m_proxyModel, &CatalogTreeFilterModel::rowsInserted, this, &CatalogView::entryProxiedPositionChanged);
    connect(m_proxyModel, &CatalogTreeFilterModel::rowsRemoved, this, &CatalogView::entryProxiedPositionChanged);

    m_proxyModel->setSourceModel(m_model);
    m_browser->setModel(m_proxyModel);
    m_browser->setColumnWidth(0, m_browser->columnWidth(0) / 3);
    m_browser->setSortingEnabled(true);
    m_browser->sortByColumn(0, Qt::AscendingOrder);
    m_browser->setWordWrap(false);
    m_browser->setUniformRowHeights(true);
    m_browser->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);

    new HeaderViewMenuHandler(m_browser->header());
    m_browser->header()->restoreState(readUiState("CatalogTreeViewState"));
}

CatalogView::~CatalogView()
{
    writeUiState("CatalogTreeViewState", m_browser->header()->saveState());
}

void CatalogView::setFocus()
{
    QDockWidget::setFocus();
    m_lineEdit->selectAll();
}

void CatalogView::slotNewEntryDisplayed(const DocPosition &pos)
{
    QModelIndex item = m_proxyModel->mapFromSource(m_model->index(pos.entry, 0));
    m_browser->setCurrentIndex(item);
    m_browser->scrollTo(item);
    m_lastKnownDocPosition = pos.entry;
}

void CatalogView::setFilterRegExp()
{
    QString expr = m_lineEdit->text();
    if (m_proxyModel->filterRegularExpression().pattern() != expr)
        m_proxyModel->setFilterRegularExpression(m_proxyModel->filterOptions() & CatalogTreeFilterModel::IgnoreAccel ? expr.remove(Project::instance()->accel())
                                                                                                                     : expr);
    refreshCurrentIndex();
}

void CatalogView::refreshCurrentIndex()
{
    QModelIndex newPositionOfSelectedItem = m_proxyModel->mapFromSource(m_model->index(m_lastKnownDocPosition, 0));
    m_browser->setCurrentIndex(newPositionOfSelectedItem);
    m_browser->scrollTo(newPositionOfSelectedItem);
}

void CatalogView::slotItemActivated(const QModelIndex &idx)
{
    Q_EMIT gotoEntry(DocPosition(m_proxyModel->mapToSource(idx).row()), 0);
}

void CatalogView::filterOptionToggled(QAction *action)
{
    if (action->data().isNull())
        return;

    int opt = action->data().toInt();
    if (opt > 0)
        m_proxyModel->setFilterOptions(m_proxyModel->filterOptions() ^ opt);
    else {
        if (opt != -1)
            opt = -opt - 2;
        m_proxyModel->setFilterKeyColumn(opt);
    }
    m_filterOptionsMenu->clear();
    refreshCurrentIndex();
}
void CatalogView::fillFilterOptionsMenu()
{
    m_filterOptionsMenu->clear();

    if (m_proxyModel->individualRejectFilterEnabled())
        m_filterOptionsMenu->addAction(i18n("Reset individual filter"), this, SLOT(setEntriesFilteredOut()));

    bool extStates = m_model->catalog()->capabilities() & ExtendedStates;

    const QStringList basicTitles = {
        i18n("Case insensitive"),
        i18n("Ignore accelerator marks"),
        i18n("Ready"),
        i18n("Non-ready"),
        i18n("Non-empty"),
        i18n("Empty"),
        i18n("Changed since file open"),
        i18n("Unchanged since file open"),
        i18n("Same in sync file"),
        i18n("Different in sync file"),
        i18n("Not in sync file"),
        i18n("Plural"),
        i18n("Non-plural"),
    };
    const QStringList extTitles = Catalog::translatedStates();
    const QStringList alltitles[2] = {basicTitles, extTitles};

    QMenu *basicMenu = m_filterOptionsMenu->addMenu(i18nc("@title:inmenu", "Basic"));
    QMenu *extMenu = extStates ? m_filterOptionsMenu->addMenu(i18nc("@title:inmenu", "States")) : nullptr;
    QMenu *allmenus[2] = {basicMenu, extMenu};
    QMenu *columnsMenu = m_filterOptionsMenu->addMenu(i18nc("@title:inmenu user selects a column to search within from a menu", "Search Column"));

    QActionGroup *columnsMenuGroup = new QActionGroup(columnsMenu);
    QAction *txt;
    txt = m_filterOptionsMenu->addAction(i18nc("@title:inmenu", "Re-sort and refilter on content change"),
                                         m_proxyModel,
                                         &CatalogTreeFilterModel::setDynamicSortFilter);
    txt->setCheckable(true);
    txt->setChecked(m_proxyModel->dynamicSortFilter());

    for (int i = 0; (1 << i) < CatalogTreeFilterModel::MaxOption; ++i) {
        bool ext = (1 << i) >= CatalogTreeFilterModel::New;
        if (!extStates && ext)
            break;
        txt = allmenus[ext]->addAction(alltitles[ext][i - ext * FIRSTSTATEPOSITION]);
        txt->setData(1 << i);
        txt->setCheckable(true);
        txt->setChecked(m_proxyModel->filterOptions() & (1 << i));
        if ((1 << i) == CatalogTreeFilterModel::IgnoreAccel)
            basicMenu->addSeparator();
    }
    if (!extStates)
        m_filterOptionsMenu->addSeparator();
    for (int i = -1; i < CatalogTreeModel::DisplayedColumnCount - 1; ++i) {
        txt = columnsMenu->addAction((i == -1) ? i18nc("@item:inmenu all columns", "All") : m_model->headerData(i, Qt::Horizontal, Qt::DisplayRole).toString());
        txt->setData(-i - 2);
        txt->setCheckable(true);
        txt->setChecked(m_proxyModel->filterKeyColumn() == i);
        txt->setActionGroup(columnsMenuGroup);
    }
    refreshCurrentIndex();
}

void CatalogView::reset()
{
    m_proxyModel->setFilterKeyColumn(-1);
    m_proxyModel->setFilterOptions(CatalogTreeFilterModel::AllStates);
    m_lineEdit->clear();
    refreshCurrentIndex();
    slotItemActivated(m_browser->currentIndex());
}

void CatalogView::setMergeCatalogPointer(MergeCatalog *pointer)
{
    m_proxyModel->setMergeCatalogPointer(pointer);
}

int CatalogView::siblingEntryNumber(int step)
{
    QModelIndex item = m_browser->currentIndex();
    int lastRow = m_proxyModel->rowCount() - 1;
    if (!item.isValid()) {
        if (lastRow == -1)
            return -1;
        item = m_proxyModel->index((step == 1) ? 0 : lastRow, 0);
        m_browser->setCurrentIndex(item);
    } else {
        if (item.row() == ((step == -1) ? 0 : lastRow))
            return -1;
        item = item.sibling(item.row() + step, 0);
    }
    return m_proxyModel->mapToSource(item).row();
}

int CatalogView::nextEntryNumber()
{
    return siblingEntryNumber(1);
}

int CatalogView::prevEntryNumber()
{
    return siblingEntryNumber(-1);
}

// Return the corresponding source model index for `row` within `m_proxyModel`.
static int edgeEntry(CatalogTreeFilterModel *m_proxyModel, int row)
{
    if (!m_proxyModel->rowCount())
        return -1;

    return m_proxyModel->mapToSource(m_proxyModel->index(row, 0)).row();
}

int CatalogView::firstEntryNumber()
{
    return edgeEntry(m_proxyModel, 0);
}

int CatalogView::lastEntryNumber()
{
    return edgeEntry(m_proxyModel, m_proxyModel->rowCount() - 1);
}

void CatalogView::setEntryFilteredOut(int entry, bool filteredOut)
{
    m_proxyModel->setEntryFilteredOut(entry, filteredOut);
    refreshCurrentIndex();
}

void CatalogView::setEntriesFilteredOut(bool filteredOut)
{
    show();
    m_proxyModel->setEntriesFilteredOut(filteredOut);
    refreshCurrentIndex();
}

#include "moc_cataloglistview.cpp"
