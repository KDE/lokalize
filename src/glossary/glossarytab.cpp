/*
  This file is part of Lokalize

  SPDX-FileCopyrightText: 2007-2014 Nick Shaforostoff <shafff@ukr.net>
  SPDX-FileCopyrightText: 2018-2019 Simon Depiets <sdepiets@gmail.com>
  SPDX-FileCopyrightText: 2025      Finley Watson <fin-w@tutanota.com>
  SPDX-FileCopyrightText: 2026      Jaimukund Bhan <bhanjaimukund@gmail.com>

  SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#include "glossarytab.h"
#include "glossary.h"
#include "languagelistmodel.h"
#include "lokalizetabpagebase.h"
#include "project.h"
#include "projectbase.h"
#include "ui_termedit.h"

#include <KLocalizedString>
#include <KMessageBox>
#include <KStandardGuiItem>
#include <kcoreaddons_version.h>

#include <QAbstractItemModel>
#include <QApplication>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QIcon>
#include <QLineEdit>
#include <QPushButton>
#include <QShortcut>
#include <QSortFilterProxyModel>
#include <QSplitter>
#include <QVBoxLayout>
#include <qtmetamacros.h>

using namespace GlossaryNS;

// BEGIN GlossaryTreeView

GlossaryTreeView::GlossaryTreeView(QWidget *parent)
    : QTreeView(parent)
{
    setSortingEnabled(true);
    sortByColumn(GlossaryModel::English, Qt::AscendingOrder);
    setItemsExpandable(false);
    setAllColumnsShowFocus(true);
}

static QByteArray modelIndexToId(const QModelIndex &item)
{
    return item.sibling(item.row(), 0).data(Qt::DisplayRole).toByteArray();
}

void GlossaryTreeView::currentChanged(const QModelIndex &current, /*previous*/ const QModelIndex &)
{
    if (current.isValid()) {
        Q_EMIT currentChanged(modelIndexToId(current));
        scrollTo(current);
    } else {
        Q_EMIT currentChanged(QByteArray());
    }
}

void GlossaryTreeView::selectRow(int i)
{
    QSortFilterProxyModel *proxyModel = static_cast<QSortFilterProxyModel *>(model());
    GlossaryModel *sourceModel = static_cast<GlossaryModel *>(proxyModel->sourceModel());

    setCurrentIndex(proxyModel->mapFromSource(sourceModel->index(i, 0)));
}

// END GlossaryTreeView

// BEGIN GlossaryTab

GlossaryTab::GlossaryTab(QWidget *parent)
    : LokalizeTabPageBaseNoQMainWindow(parent)
    , m_browser(new GlossaryTreeView(this))
    , m_proxyModel(new GlossarySortFilterProxyModel(this))
    , m_reactOnSignals(true)
{
    m_tabIcon = QIcon::fromTheme(QStringLiteral("view-list-text"));
    m_tabLabel = i18nc("@title", "Glossary");

    setAttribute(Qt::WA_DeleteOnClose, false);
    setContentsMargins(0, 0, 0, 0);

    QVBoxLayout *wrapperLayout = new QVBoxLayout(this);
    wrapperLayout->setContentsMargins(0, 0, 0, 0);

    QSplitter *splitter = new QSplitter(Qt::Horizontal, this);
    wrapperLayout->addWidget(splitter);

    m_proxyModel->setFilterKeyColumn(-1);
    m_proxyModel->setDynamicSortFilter(true);
    GlossaryModel *model = new GlossaryModel(this);
    m_proxyModel->setSourceModel(model);
    m_browser->setModel(m_proxyModel);

    m_browser->setUniformRowHeights(true);
    m_browser->setAutoScroll(true);
    m_browser->setColumnHidden(GlossaryModel::ID, true);
    m_browser->setColumnWidth(GlossaryModel::English, m_browser->columnWidth(GlossaryModel::English) * 2); // man this is  HACK y
    m_browser->setColumnWidth(GlossaryModel::Target, m_browser->columnWidth(GlossaryModel::Target) * 2);
    m_browser->setAlternatingRowColors(true);
    m_browser->setContentsMargins(0, 0, 0, 0);

    // left
    QWidget *w = new QWidget(splitter);
    QVBoxLayout *layout = new QVBoxLayout(w);
    layout->setContentsMargins(QMargins(style()->pixelMetric(QStyle::PM_LayoutHorizontalSpacing),
                                        style()->pixelMetric(QStyle::PM_LayoutVerticalSpacing),
                                        style()->pixelMetric(QStyle::PM_LayoutHorizontalSpacing),
                                        style()->pixelMetric(QStyle::PM_LayoutVerticalSpacing)));
    m_filterEdit = new QLineEdit(w);
    m_filterEdit->setClearButtonEnabled(true);
    m_filterEdit->setPlaceholderText(i18n("Search"));
    m_filterEdit->setFocus();
    m_filterEdit->setToolTip(i18nc("@info:tooltip", "Activated by Ctrl+L. Accepts regular expressions"));
    m_filterEdit->setContentsMargins(0, 0, 0, 0);
    new QShortcut(QKeySequence(Qt::ControlModifier | Qt::Key_L), this, SLOT(setFocus()), nullptr, Qt::WidgetWithChildrenShortcut);
    connect(m_filterEdit, &QLineEdit::textChanged, m_proxyModel, &GlossaryNS::GlossarySortFilterProxyModel::setFilterRegExp);

    layout->addWidget(m_filterEdit);
    layout->addWidget(m_browser);
    {
        QPushButton *addBtn = new QPushButton(w);
        connect(addBtn, &QPushButton::clicked, this, qOverload<>(&GlossaryTab::newTermEntry));

        QPushButton *rmBtn = new QPushButton(w);
        connect(rmBtn, &QPushButton::clicked, this, qOverload<>(&GlossaryTab::rmTermEntry));
        KGuiItem::assign(addBtn, KStandardGuiItem::add());
        KGuiItem::assign(rmBtn, KStandardGuiItem::remove());
        addBtn->setToolTip(i18nc("@info:tooltip", "Add new glossary entry"));
        rmBtn->setToolTip(i18nc("@info:tooltip", "Remove selected glossary entry"));

        QWidget *btns = new QWidget(w);
        QHBoxLayout *btnsLayout = new QHBoxLayout(btns);
        btnsLayout->setContentsMargins(0, 0, 0, 0);
        btnsLayout->addWidget(addBtn);
        btnsLayout->addWidget(rmBtn);

        layout->addWidget(btns);
        QWidget::setTabOrder(addBtn, rmBtn);
        QWidget::setTabOrder(rmBtn, m_filterEdit);
    }
    QWidget::setTabOrder(m_filterEdit, m_browser);

    splitter->addWidget(w);

    // right
    m_editor = new QWidget(splitter);
    Ui_TermEdit ui_termEdit;
    ui_termEdit.setupUi(m_editor);
    // This must come after the call to setupUi() because it sets margins too, the below is correct though.
    m_editor->setContentsMargins(0, 0, 0, 0);
    splitter->addWidget(m_editor);

    Project *project = Project::instance();
    m_sourceTermsModel = new TermsListModel(project->glossary(), project->sourceLangCode(), this);
    m_targetTermsModel = new TermsListModel(project->glossary(), project->targetLangCode(), this);

    ui_termEdit.sourceTermsView->setModel(m_sourceTermsModel);
    ui_termEdit.targetTermsView->setModel(m_targetTermsModel);

    connect(ui_termEdit.addEngTerm, &QPushButton::clicked, ui_termEdit.sourceTermsView, &TermListView::addTerm);
    connect(ui_termEdit.remEngTerm, &QPushButton::clicked, ui_termEdit.sourceTermsView, &TermListView::rmTerms);
    connect(ui_termEdit.addTargetTerm, &QPushButton::clicked, ui_termEdit.targetTermsView, &TermListView::addTerm);
    connect(ui_termEdit.remTargetTerm, &QPushButton::clicked, ui_termEdit.targetTermsView, &TermListView::rmTerms);

    m_sourceTermsView = ui_termEdit.sourceTermsView;
    m_targetTermsView = ui_termEdit.targetTermsView;
    m_subjectField = ui_termEdit.subjectField;
    m_definition = ui_termEdit.definition;
    m_definitionLang = ui_termEdit.definitionLang;

    connect(m_subjectField->lineEdit(), &QLineEdit::editingFinished, this, &GlossaryTab::applyEntryChange);

    QStringList subjectFields = Project::instance()->glossary()->subjectFields();
    std::sort(subjectFields.begin(), subjectFields.end());
    QStringListModel *subjectFieldsModel = new QStringListModel(this);
    subjectFieldsModel->setStringList(subjectFields);
    m_subjectField->setModel(subjectFieldsModel);
    connect(m_browser, qOverload<int>(&GlossaryTreeView::currentChanged), this, &GlossaryTab::currentChanged);
    connect(m_browser, qOverload<const QByteArray &>(&GlossaryTreeView::currentChanged), this, &GlossaryTab::showEntryInEditor);

    connect(m_definitionLang, qOverload<int>(&KComboBox::activated), this, &GlossaryTab::showDefinitionForLang);
    m_definitionLang->setModel(LanguageListModel::emptyLangInstance()->sortModel());
    m_definitionLang->setCurrentIndex(LanguageListModel::emptyLangInstance()->sortModelRowForLangCode(m_defLang)); // empty lang

    connect(m_definition, &AuxTextEdit::editingFinished, this, &GlossaryTab::applyEntryChange);

    QModelIndex firstVisualItem = m_browser->model()->index(0, 0);
    if (firstVisualItem.isValid()) {
        m_browser->setCurrentIndex(firstVisualItem);
        m_browser->scrollTo(firstVisualItem);
    }
}

void GlossaryTab::setFocus()
{
    m_filterEdit->setFocus();
    m_filterEdit->selectAll();
}

void GlossaryTab::showEntryInEditor(const QByteArray &id)
{
    if (m_editor->isVisible())
        applyEntryChange();
    else
        m_editor->show();

    m_id = id;

    m_reactOnSignals = false;

    Project *project = Project::instance();
    Glossary *glossary = project->glossary();
    m_subjectField->setCurrentItem(glossary->subjectField(id), /*insert*/ true);

    const QStringList langsToTry = QStringList(m_defLang) << QStringLiteral("en") << QStringLiteral("en_US") << project->targetLangCode();
    for (const QString &lang : langsToTry) {
        QString d = glossary->definition(m_id, lang);
        if (!d.isEmpty()) {
            if (m_defLang != lang)
                m_definitionLang->setCurrentIndex(LanguageListModel::emptyLangInstance()->sortModelRowForLangCode(lang));
            m_defLang = lang;
            break;
        }
    }
    m_definition->setPlainText(glossary->definition(m_id, m_defLang));

    m_sourceTermsModel->setEntry(id);
    m_targetTermsModel->setEntry(id);

    m_reactOnSignals = true;
}

void GlossaryTab::currentChanged(int i)
{
    Q_UNUSED(i);
    m_reactOnSignals = false;
    m_editor->show();
    m_reactOnSignals = true;
}

void GlossaryTab::showDefinitionForLang(int langModelIndex)
{
    applyEntryChange();
    m_defLang = LanguageListModel::emptyLangInstance()->langCodeForSortModelRow(langModelIndex);
    m_definition->setPlainText(Project::instance()->glossary()->definition(m_id, m_defLang));
}

void GlossaryTab::applyEntryChange()
{
    if (!m_reactOnSignals || !m_browser->currentIndex().isValid())
        return;

    QByteArray id = m_id; // modelIndexToId(m_browser->currentIndex());

    Project *project = Project::instance();
    Glossary *glossary = project->glossary();

    if (m_subjectField->currentText() != glossary->subjectField(id))
        glossary->setSubjectField(id, QString(), m_subjectField->currentText());

    if (m_definition->toPlainText() != glossary->definition(id, m_defLang))
        glossary->setDefinition(id, m_defLang, m_definition->toPlainText());

    // HACK to force finishing of the listview editing
    QWidget *prevFocusWidget = QApplication::focusWidget();
    m_browser->setFocus();
    if (prevFocusWidget)
        prevFocusWidget->setFocus();

    const QModelIndex &idx = m_proxyModel->mapToSource(m_browser->currentIndex());
    if (!idx.isValid())
        return;
}

void GlossaryTab::selectEntry(const QByteArray &id)
{
    // let it fetch the rows
    QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents | QEventLoop::ExcludeSocketNotifiers | QEventLoop::WaitForMoreEvents, 100);

    QModelIndexList items = m_proxyModel->match(m_proxyModel->index(0, 0), Qt::DisplayRole, QVariant(id), 1, Qt::MatchExactly);
    if (items.count()) {
        m_browser->setCurrentIndex(items.first());
        m_browser->scrollTo(items.first(), QAbstractItemView::PositionAtCenter);
    } else {
        // the row is probably not fetched yet
        m_browser->setCurrentIndex(QModelIndex());
        showEntryInEditor(id);
    }
    Q_EMIT signalActivateGlossaryTab();
}

void GlossaryTab::newTermEntry()
{
    QDialog dialog(this);
    dialog.setWindowTitle(i18nc("@title:window", "Add New Term"));
    dialog.setMinimumWidth(400);

    QVBoxLayout *mainLayout = new QVBoxLayout(&dialog);
    QFormLayout *formLayout = new QFormLayout();

    QLineEdit *sourceEdit = new QLineEdit(&dialog);
    QLineEdit *targetEdit = new QLineEdit(&dialog);

    formLayout->addRow(i18n("English Term:"), sourceEdit);
    formLayout->addRow(i18n("Target Term:"), targetEdit);
    mainLayout->addLayout(formLayout);
    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
    mainLayout->addWidget(buttonBox);

    connect(buttonBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

    if (dialog.exec() == QDialog::Accepted) {
        QString sourceText = sourceEdit->text().trimmed();
        QString targetText = targetEdit->text().trimmed();

        if (!sourceText.isEmpty() || !targetText.isEmpty()) {
            newTermEntry(sourceText, targetText);
        }
    }
}

void GlossaryTab::newTermEntry(QString _source, QString _target)
{
    GlossaryModel *sourceModel = static_cast<GlossaryModel *>(m_proxyModel->sourceModel());
    QByteArray id = sourceModel->appendRow(_source, _target);

    selectEntry(id);
    Project::instance()->glossary()->save();
}

void GlossaryTab::rmTermEntry()
{
    rmTermEntry(-1);
}

void GlossaryTab::rmTermEntry(int i)
{
    GlossaryModel *sourceModel = static_cast<GlossaryModel *>(m_proxyModel->sourceModel());

    if (i == -1) {
        // NOTE actually we should remove selected items, not current one
        const QModelIndex &current = m_browser->currentIndex();
        if (!current.isValid())
            return;
        i = m_proxyModel->mapToSource(current).row();
    }

    sourceModel->removeRow(i);
    Project::instance()->glossary()->save();
}

// END GlossaryTab

void TermsListModel::setEntry(const QByteArray &id)
{
    m_id = id;
    QStringList terms = m_glossary->terms(m_id, m_lang);
    terms.append(QString()); // allow adding new terms
    setStringList(terms);
}

bool TermsListModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    Q_UNUSED(role);
    m_glossary->setTerm(m_id, m_lang, index.row(), value.toString());
    setEntry(m_id); // allow adding new terms
    return true;
}

bool TermsListModel::removeRows(int row, int count, const QModelIndex &parent)
{
    Q_UNUSED(count)
    if (row == rowCount() - 1)
        return false; // cannot delete non-existing item

    m_glossary->rmTerm(m_id, m_lang, row);
    return QStringListModel::removeRows(row, 1, parent);
}

void TermListView::addTerm()
{
    setCurrentIndex(model()->index(model()->rowCount() - 1, 0));
    edit(currentIndex());
}

void TermListView::rmTerms()
{
    const auto rows = selectionModel()->selectedRows();
    for (const QModelIndex &row : rows)
        model()->removeRow(row.row());
}

#include "moc_glossarytab.cpp"
