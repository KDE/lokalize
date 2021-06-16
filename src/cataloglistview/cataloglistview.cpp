/* ****************************************************************************
  This file is part of Lokalize

  Copyright (C) 2007-2009 by Nick Shaforostoff <shafff@ukr.net>
                2018-2019 by Simon Depiets <sdepiets@gmail.com>

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License as
  published by the Free Software Foundation; either version 2 of
  the License or (at your option) version 3 or any later version
  accepted by the membership of KDE e.V. (or its successor approved
  by the membership of KDE e.V.), which shall act as a proxy
  defined in Section 14 of version 3 of the license.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.

**************************************************************************** */

#include "cataloglistview.h"

#include "lokalize_debug.h"

#include "catalogmodel.h"
#include "catalog.h"
#include "project.h"
#include "prefs.h"
#include "headerviewmenu.h"

#include <QLineEdit>
#include <QTime>
#include <QTreeView>
#include <QHeaderView>
#include <QModelIndex>
#include <QToolButton>
#include <QVBoxLayout>
#include <QAction>
#include <QMenu>
#include <QShortcut>
#include <QKeyEvent>

#include <KLocalizedString>
#include <KConfigGroup>

class CatalogTreeView: public QTreeView
{
public:
    CatalogTreeView(QWidget * parent)
        : QTreeView(parent) {}
    ~CatalogTreeView() {}

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


CatalogView::CatalogView(QWidget* parent, Catalog* catalog)
    : QDockWidget(i18nc("@title:window aka Message Tree", "Translation Units"), parent)
    , m_browser(new CatalogTreeView(this))
    , m_lineEdit(new QLineEdit(this))
    , m_model(new CatalogTreeModel(this, catalog))
    , m_proxyModel(new CatalogTreeFilterModel(this))
{
    setObjectName(QStringLiteral("catalogTreeView"));

    QWidget* w = new QWidget(this);
    QVBoxLayout* layout = new QVBoxLayout(w);
    layout->setContentsMargins(0, 0, 0, 0);
    QHBoxLayout* l = new QHBoxLayout;
    l->setContentsMargins(0, 0, 0, 0);
    l->setSpacing(0);
    layout->addLayout(l);

    m_lineEdit->setClearButtonEnabled(true);
    m_lineEdit->setPlaceholderText(i18n("Quick search..."));
    m_lineEdit->setToolTip(i18nc("@info:tooltip", "Activated by Ctrl+L.") + ' ' + i18nc("@info:tooltip", "Accepts regular expressions"));
    connect(m_lineEdit, &QLineEdit::textChanged, this, &CatalogView::setFilterRegExp, Qt::QueuedConnection);
    // QShortcut* ctrlEsc=new QShortcut(QKeySequence(Qt::META+Qt::Key_Escape),this,SLOT(reset()),0,Qt::WidgetWithChildrenShortcut);
    QShortcut* esc = new QShortcut(QKeySequence(Qt::Key_Escape), this, nullptr, nullptr, Qt::WidgetWithChildrenShortcut);
    connect(esc, &QShortcut::activated, this, &CatalogView::escaped);


    QToolButton* btn = new QToolButton(w);
    btn->setPopupMode(QToolButton::InstantPopup);
    btn->setText(i18n("options"));
    //btn->setArrowType(Qt::DownArrow);
    btn->setMenu(new QMenu(this));
    m_filterOptionsMenu = btn->menu();
    connect(m_filterOptionsMenu, &QMenu::aboutToShow, this, &CatalogView::fillFilterOptionsMenu);
    connect(m_filterOptionsMenu, &QMenu::triggered, this, &CatalogView::filterOptionToggled);

    l->addWidget(m_lineEdit);
    l->addWidget(btn);
    layout->addWidget(m_browser);


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

void CatalogView::slotNewEntryDisplayed(const DocPosition& pos)
{
    QModelIndex item = m_proxyModel->mapFromSource(m_model->index(pos.entry, 0));
    m_browser->setCurrentIndex(item);
    m_browser->scrollTo(item/*,QAbstractItemView::PositionAtCenter*/);
    m_lastKnownDocPosition = pos.entry;
}

void CatalogView::setFilterRegExp()
{
    QString expr = m_lineEdit->text();
    if (m_proxyModel->filterRegExp().pattern() != expr)
        m_proxyModel->setFilterRegExp(m_proxyModel->filterOptions()&CatalogTreeFilterModel::IgnoreAccel ? expr.remove(Project::instance()->accel()) : expr);
    refreshCurrentIndex();
}

void CatalogView::refreshCurrentIndex()
{
    QModelIndex newPositionOfSelectedItem = m_proxyModel->mapFromSource(m_model->index(m_lastKnownDocPosition, 0));
    m_browser->setCurrentIndex(newPositionOfSelectedItem);
    m_browser->scrollTo(newPositionOfSelectedItem);
}

void CatalogView::slotItemActivated(const QModelIndex& idx)
{
    Q_EMIT gotoEntry(DocPosition(m_proxyModel->mapToSource(idx).row()), 0);
}

void CatalogView::filterOptionToggled(QAction* action)
{
    if (action->data().isNull())
        return;

    int opt = action->data().toInt();
    if (opt > 0)
        m_proxyModel->setFilterOptions(m_proxyModel->filterOptions()^opt);
    else {
        if (opt != -1) opt = -opt - 2;
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


    bool extStates = m_model->catalog()->capabilities()&ExtendedStates;

    const char* const basicTitles[] = {
        I18N_NOOP("Case insensitive"),
        I18N_NOOP("Ignore accelerator marks"),
        I18N_NOOP("Ready"),
        I18N_NOOP("Non-ready"),
        I18N_NOOP("Non-empty"),
        I18N_NOOP("Empty"),
        I18N_NOOP("Changed since file open"),
        I18N_NOOP("Unchanged since file open"),
        I18N_NOOP("Same in sync file"),
        I18N_NOOP("Different in sync file"),
        I18N_NOOP("Not in sync file"),
        I18N_NOOP("Plural"),
        I18N_NOOP("Non-plural"),
    };
    const char* const* extTitles = Catalog::states();
    const char* const* alltitles[2] = {basicTitles, extTitles};

    QMenu* basicMenu = m_filterOptionsMenu->addMenu(i18nc("@title:inmenu", "Basic"));
    QMenu* extMenu = extStates ? m_filterOptionsMenu->addMenu(i18nc("@title:inmenu", "States")) : nullptr;
    QMenu* allmenus[2] = {basicMenu, extMenu};
    QMenu* columnsMenu = m_filterOptionsMenu->addMenu(i18nc("@title:inmenu", "Searchable column"));

    QActionGroup* columnsMenuGroup = new QActionGroup(columnsMenu);
    QAction* txt;
    txt = m_filterOptionsMenu->addAction(i18nc("@title:inmenu", "Resort and refilter on content change"), m_proxyModel, &CatalogTreeFilterModel::setDynamicSortFilter);
    txt->setCheckable(true);
    txt->setChecked(m_proxyModel->dynamicSortFilter());

    for (int i = 0; (1 << i) < CatalogTreeFilterModel::MaxOption; ++i) {
        bool ext = (1 << i) >= CatalogTreeFilterModel::New;
        if (!extStates && ext) break;
        txt = allmenus[ext]->addAction(i18n(alltitles[ext][i - ext * FIRSTSTATEPOSITION]));
        txt->setData(1 << i);
        txt->setCheckable(true);
        txt->setChecked(m_proxyModel->filterOptions() & (1 << i));
        if ((1 << i) == CatalogTreeFilterModel::IgnoreAccel)
            basicMenu->addSeparator();
    }
    if (!extStates)
        m_filterOptionsMenu->addSeparator();
    for (int i = -1; i < CatalogTreeModel::DisplayedColumnCount - 1; ++i) {
        txt = columnsMenu->addAction((i == -1) ? i18nc("@item:inmenu all columns", "All") :
                                     m_model->headerData(i, Qt::Horizontal, Qt::DisplayRole).toString());
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
    //Q_EMIT gotoEntry(DocPosition(m_proxyModel->mapToSource(m_browser->currentIndex()).row()),0);
    slotItemActivated(m_browser->currentIndex());
}

void CatalogView::setMergeCatalogPointer(MergeCatalog* pointer)
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

static int edgeEntry(CatalogTreeFilterModel* m_proxyModel, int row)
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

