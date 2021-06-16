/* ****************************************************************************
  This file is part of Lokalize

  Copyright (C) 2007-2014 by Nick Shaforostoff <shafff@ukr.net>
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

#include "glossarywindow.h"

#include "lokalize_debug.h"

#include "glossary.h"
#include "project.h"
#include "languagelistmodel.h"
#include "ui_termedit.h"

#include <klocalizedstring.h>
#include <kmessagebox.h>
#include <kstandardguiitem.h>

#include <QLineEdit>
#include <QApplication>
#include <QSplitter>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSortFilterProxyModel>
#include <QAbstractItemModel>
#include <QStringListModel>
#include <QShortcut>
#include <QPushButton>

using namespace GlossaryNS;

//BEGIN GlossaryTreeView

GlossaryTreeView::GlossaryTreeView(QWidget *parent)
    : QTreeView(parent)
{
    setSortingEnabled(true);
    sortByColumn(GlossaryModel::English, Qt::AscendingOrder);
    setItemsExpandable(false);
    setAllColumnsShowFocus(true);

    /*
        setSelectionMode(QAbstractItemView::ExtendedSelection);
        setSelectionBehavior(QAbstractItemView::SelectRows);*/

}

static QByteArray modelIndexToId(const QModelIndex& item)
{
    return item.sibling(item.row(), 0).data(Qt::DisplayRole).toByteArray();
}

void GlossaryTreeView::currentChanged(const QModelIndex& current, const QModelIndex&/* previous*/)
{
    if (current.isValid()) {
        //QModelIndex item=static_cast<QSortFilterProxyModel*>(model())->mapToSource(current);
        //Q_EMIT currentChanged(item.row());
        Q_EMIT currentChanged(modelIndexToId(current));
        scrollTo(current);
    }
}

void GlossaryTreeView::selectRow(int i)
{
    QSortFilterProxyModel* proxyModel = static_cast<QSortFilterProxyModel*>(model());
    GlossaryModel* sourceModel = static_cast<GlossaryModel*>(proxyModel->sourceModel());

    //sourceModel->forceReset();
    setCurrentIndex(proxyModel->mapFromSource(sourceModel->index(i, 0)));
}


//END GlossaryTreeView


//BEGIN SubjectFieldModel
//typedef QStringListModel SubjectFieldModel;

#if 0
class SubjectFieldModel: public QAbstractItemModel
{
public:

    //Q_OBJECT

    SubjectFieldModel(QObject* parent);
    ~SubjectFieldModel() {}

    QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const;
    QModelIndex parent(const QModelIndex&) const;
    int rowCount(const QModelIndex& parent = QModelIndex()) const;
    int columnCount(const QModelIndex& parent = QModelIndex()) const;
    QVariant data(const QModelIndex&, int role = Qt::DisplayRole) const;
    bool setData(const QModelIndex&, const QVariant&, int role = Qt::EditRole);
    bool setItemData(const QModelIndex& index, const QMap<int, QVariant>& roles);
    bool insertRows(int row, int count, const QModelIndex& parent = QModelIndex());
    Qt::ItemFlags flags(const QModelIndex&) const;

    /*private:
        Catalog* m_catalog;*/
};

inline
SubjectFieldModel::SubjectFieldModel(QObject* parent)
    : QAbstractItemModel(parent)
// , m_catalog(catalog)
{
}

QModelIndex SubjectFieldModel::index(int row, int column, const QModelIndex& /*parent*/) const
{
    return createIndex(row, column);
}

Qt::ItemFlags SubjectFieldModel::flags(const QModelIndex&) const
{
    return Qt::ItemIsSelectable | Qt::ItemIsEditable | Qt::ItemIsEnabled;
}

QModelIndex SubjectFieldModel::parent(const QModelIndex& /*index*/) const
{
    return QModelIndex();
}

int SubjectFieldModel::columnCount(const QModelIndex& parent) const
{
    if (parent.isValid())
        return 0;
    return 1;
}
/*
inline
Qt::ItemFlags SubjectFieldModel::flags ( const QModelIndex & index ) const
{
    if (index.column()==FuzzyFlag)
        return Qt::ItemIsSelectable|Qt::ItemIsUserCheckable|Qt::ItemIsEnabled;
    return QAbstractItemModel::flags(index);
}*/

int SubjectFieldModel::rowCount(const QModelIndex& parent) const
{
    if (parent.isValid())
        return 0;
    return Project::instance()->glossary()->subjectFields.size();
}

QVariant SubjectFieldModel::data(const QModelIndex& index, int role) const
{
    if (role == Qt::DisplayRole || role == Qt::EditRole)
        return Project::instance()->glossary()->subjectFields.at(index.row());
    return QVariant();
}

bool SubjectFieldModel::insertRows(int row, int count, const QModelIndex& parent)
{
    beginInsertRows(parent, row, row + count - 1);

    QStringList& subjectFields = Project::instance()->glossary()->subjectFields;

    while (--count >= 0)
        subjectFields.insert(row + count, QString());

    endInsertRows();
    return true;
}

bool SubjectFieldModel::setData(const QModelIndex& index, const QVariant& value, int role)
{
    QStringList& subjectFields = Project::instance()->glossary()->subjectFields;
    subjectFields[index.row()] = value.toString();
    return true;
}

bool SubjectFieldModel::setItemData(const QModelIndex& index, const QMap<int, QVariant>& roles)
{
    if (roles.contains(Qt::EditRole)) {
        QStringList& subjectFields = Project::instance()->glossary()->subjectFields;
        subjectFields[index.row()] = roles.value(Qt::EditRole).toString();
    }
    return true;
}
#endif
//END SubjectFieldModel

//BEGIN GlossaryWindow

GlossaryWindow::GlossaryWindow(QWidget *parent)
    : KMainWindow(parent)
    , m_browser(new GlossaryTreeView(this))
    , m_proxyModel(new GlossarySortFilterProxyModel(this))
    , m_reactOnSignals(true)
{
    //setAttribute(Qt::WA_DeleteOnClose, true);
    setAttribute(Qt::WA_DeleteOnClose, false);

    QSplitter* splitter = new QSplitter(Qt::Horizontal, this);
    setCentralWidget(splitter);

    m_proxyModel->setFilterKeyColumn(-1);
    m_proxyModel->setDynamicSortFilter(true);;
    GlossaryModel* model = new GlossaryModel(this);
    m_proxyModel->setSourceModel(model);
    m_browser->setModel(m_proxyModel);

    m_browser->setUniformRowHeights(true);
    m_browser->setAutoScroll(true);
    m_browser->setColumnHidden(GlossaryModel::ID, true);
    m_browser->setColumnWidth(GlossaryModel::English, m_browser->columnWidth(GlossaryModel::English) * 2); //man this is  HACK y
    m_browser->setColumnWidth(GlossaryModel::Target, m_browser->columnWidth(GlossaryModel::Target) * 2);
    m_browser->setAlternatingRowColors(true);

    //left
    QWidget* w = new QWidget(splitter);
    QVBoxLayout* layout = new QVBoxLayout(w);
    m_filterEdit = new QLineEdit(w);
    m_filterEdit->setClearButtonEnabled(true);
    m_filterEdit->setPlaceholderText(i18n("Quick search..."));
    m_filterEdit->setFocus();
    m_filterEdit->setToolTip(i18nc("@info:tooltip", "Activated by Ctrl+L.") + ' ' + i18nc("@info:tooltip", "Accepts regular expressions"));
    new QShortcut(Qt::CTRL + Qt::Key_L, this, SLOT(setFocus()), nullptr, Qt::WidgetWithChildrenShortcut);
    connect(m_filterEdit, &QLineEdit::textChanged, m_proxyModel, &GlossaryNS::GlossarySortFilterProxyModel::setFilterRegExp);

    layout->addWidget(m_filterEdit);
    layout->addWidget(m_browser);
    {
        QPushButton* addBtn = new QPushButton(w);
        connect(addBtn, &QPushButton::clicked, this, QOverload<>::of(&GlossaryWindow::newTermEntry));

        QPushButton* rmBtn = new QPushButton(w);
        connect(rmBtn, &QPushButton::clicked, this, QOverload<>::of(&GlossaryWindow::rmTermEntry));
        KGuiItem::assign(addBtn, KStandardGuiItem::add());
        KGuiItem::assign(rmBtn, KStandardGuiItem::remove());

        QPushButton* restoreBtn = new QPushButton(i18nc("@action:button reloads glossary from disk", "Restore from disk"), w);
        restoreBtn->setToolTip(i18nc("@info:tooltip", "Reload glossary from disk, discarding any changes"));
        connect(restoreBtn, &QPushButton::clicked, this, &GlossaryWindow::restore);

        QWidget* btns = new QWidget(w);
        QHBoxLayout* btnsLayout = new QHBoxLayout(btns);
        btnsLayout->addWidget(addBtn);
        btnsLayout->addWidget(rmBtn);
        btnsLayout->addWidget(restoreBtn);

        layout->addWidget(btns);
        //QWidget::setTabOrder(m_browser,addBtn);
        QWidget::setTabOrder(addBtn, rmBtn);
        QWidget::setTabOrder(rmBtn, restoreBtn);
        QWidget::setTabOrder(restoreBtn, m_filterEdit);
    }
    QWidget::setTabOrder(m_filterEdit, m_browser);

    splitter->addWidget(w);

    //right
    m_editor = new QWidget(splitter);
    m_editor->hide();
    Ui_TermEdit ui_termEdit;
    ui_termEdit.setupUi(m_editor);
    splitter->addWidget(m_editor);


    Project* project = Project::instance();
    m_sourceTermsModel = new TermsListModel(project->glossary(), project->sourceLangCode(), this);
    m_targetTermsModel = new TermsListModel(project->glossary(), project->targetLangCode(), this);

    ui_termEdit.sourceTermsView->setModel(m_sourceTermsModel);
    ui_termEdit.targetTermsView->setModel(m_targetTermsModel);

    connect(ui_termEdit.addEngTerm, &QToolButton::clicked, ui_termEdit.sourceTermsView, &TermListView::addTerm);
    connect(ui_termEdit.remEngTerm, &QToolButton::clicked, ui_termEdit.sourceTermsView, &TermListView::rmTerms);
    connect(ui_termEdit.addTargetTerm, &QToolButton::clicked, ui_termEdit.targetTermsView, &TermListView::addTerm);
    connect(ui_termEdit.remTargetTerm, &QToolButton::clicked, ui_termEdit.targetTermsView, &TermListView::rmTerms);

    m_sourceTermsView = ui_termEdit.sourceTermsView;
    m_targetTermsView = ui_termEdit.targetTermsView;
    m_subjectField = ui_termEdit.subjectField;
    m_definition = ui_termEdit.definition;
    m_definitionLang = ui_termEdit.definitionLang;

    //connect (m_english,SIGNAL(textChanged()),   this,SLOT(applyEntryChange()));
    //connect (m_target,SIGNAL(textChanged()),    this,SLOT(applyEntryChange()));
    //connect (m_definition,SIGNAL(editingFinished()),this,SLOT(applyEntryChange()));
    //connect (m_definition,SIGNAL(textChanged()),this,SLOT(applyEntryChange()));
    //connect (m_subjectField,SIGNAL(editTextChanged(QString)),this,SLOT(applyEntryChange()));
    connect(m_subjectField->lineEdit(), &QLineEdit::editingFinished, this, &GlossaryWindow::applyEntryChange);


    //m_subjectField->addItems(Project::instance()->glossary()->subjectFields());
    //m_subjectField->setModel(new SubjectFieldModel(this));
    QStringList subjectFields = Project::instance()->glossary()->subjectFields();
    std::sort(subjectFields.begin(), subjectFields.end());
    QStringListModel* subjectFieldsModel = new QStringListModel(this);
    subjectFieldsModel->setStringList(subjectFields);
    m_subjectField->setModel(subjectFieldsModel);
    connect(m_browser, QOverload<int>::of(&GlossaryTreeView::currentChanged), this, &GlossaryWindow::currentChanged);
    connect(m_browser, QOverload<const QByteArray &>::of(&GlossaryTreeView::currentChanged), this, &GlossaryWindow::showEntryInEditor);

    connect(m_definitionLang, QOverload<int>::of(&KComboBox::activated), this, &GlossaryWindow::showDefinitionForLang);
    m_definitionLang->setModel(LanguageListModel::emptyLangInstance()->sortModel());
    m_definitionLang->setCurrentIndex(LanguageListModel::emptyLangInstance()->sortModelRowForLangCode(m_defLang));//empty lang

    //TODO
    //connect(m_targetTermsModel,SIGNAL(dataChanged(QModelIndex,QModelIndex)),m_browser,SLOT(setFocus()));

    setAutoSaveSettings(QLatin1String("GlossaryWindow"), true);
    //Glossary* glossary=Project::instance()->glossary();
    /*setCaption(i18nc("@title:window","Glossary"),
              !glossary->changedIds.isEmpty()||!glossary->addedIds.isEmpty()||!glossary->removedIds.isEmpty());
              */
}

void GlossaryWindow::setFocus()
{
    m_filterEdit->setFocus();
    m_filterEdit->selectAll();
}


void GlossaryWindow::showEntryInEditor(const QByteArray& id)
{
    if (m_editor->isVisible())
        applyEntryChange();
    else
        m_editor->show();

    m_id = id;

    m_reactOnSignals = false;

    Project* project = Project::instance();
    Glossary* glossary = project->glossary();
    m_subjectField->setCurrentItem(glossary->subjectField(id),/*insert*/true);

    const QStringList langsToTry = QStringList(m_defLang) << QStringLiteral("en") << QStringLiteral("en_US") << project->targetLangCode();
    for (const QString& lang : langsToTry) {
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

    //m_sourceTermsModel->setStringList(glossary->terms(id,project->sourceLangCode()));
    //m_targetTermsModel->setStringList(glossary->terms(id,project->targetLangCode()));

    m_reactOnSignals = true;
}

void GlossaryWindow::currentChanged(int i)
{
    Q_UNUSED(i);
    m_reactOnSignals = false;
    m_editor->show();
    m_reactOnSignals = true;
}

void GlossaryWindow::showDefinitionForLang(int langModelIndex)
{
    applyEntryChange();
    m_defLang = LanguageListModel::emptyLangInstance()->langCodeForSortModelRow(langModelIndex);
    m_definition->setPlainText(Project::instance()->glossary()->definition(m_id, m_defLang));
}

void GlossaryWindow::applyEntryChange()
{
    if (!m_reactOnSignals || !m_browser->currentIndex().isValid())
        return;

    QByteArray id = m_id; //modelIndexToId(m_browser->currentIndex());

    Project* project = Project::instance();
    Glossary* glossary = project->glossary();

    if (m_subjectField->currentText() != glossary->subjectField(id))
        glossary->setSubjectField(id, QString(), m_subjectField->currentText());

    if (m_definition->toPlainText() != glossary->definition(id, m_defLang))
        glossary->setDefinition(id, m_defLang, m_definition->toPlainText());

    //HACK to force finishing of the listview editing
    QWidget* prevFocusWidget = QApplication::focusWidget();
    m_browser->setFocus();
    if (prevFocusWidget)
        prevFocusWidget->setFocus();

//  QSortFilterProxyModel* proxyModel=static_cast<QSortFilterProxyModel*>(model());
    //GlossaryModel* sourceModel=static_cast<GlossaryModel*>(m_proxyModel->sourceModel());
    const QModelIndex& idx = m_proxyModel->mapToSource(m_browser->currentIndex());
    if (!idx.isValid())
        return;


    //TODO display filename, optionally stripped like for filetab names
    setCaption(i18nc("@title:window", "Glossary"), !glossary->isClean());
}


void GlossaryWindow::selectEntry(const QByteArray& id)
{
    //let it fetch the rows
    QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents | QEventLoop::ExcludeSocketNotifiers | QEventLoop::WaitForMoreEvents, 100);

    QModelIndexList items = m_proxyModel->match(m_proxyModel->index(0, 0), Qt::DisplayRole, QVariant(id), 1, Qt::MatchExactly);
    if (items.count()) {
        m_browser->setCurrentIndex(items.first());
        m_browser->scrollTo(items.first(), QAbstractItemView::PositionAtCenter);
    } else {
        //the row is probably not fetched yet
        m_browser->setCurrentIndex(QModelIndex());
        showEntryInEditor(id);
    }
}

void GlossaryWindow::newTermEntry()
{
    newTermEntry(QString(), QString());
}

void GlossaryWindow::newTermEntry(QString _english, QString _target)
{
    setCaption(i18nc("@title:window", "Glossary"), true);

    GlossaryModel* sourceModel = static_cast<GlossaryModel*>(m_proxyModel->sourceModel());
    QByteArray id = sourceModel->appendRow(_english, _target);

    selectEntry(id);
}

void GlossaryWindow::rmTermEntry()
{
    rmTermEntry(-1);
}

void GlossaryWindow::rmTermEntry(int i)
{
    setCaption(i18nc("@title:window", "Glossary"), true);

    //QSortFilterProxyModel* proxyModel=static_cast<QSortFilterProxyModel*>(model());
    GlossaryModel* sourceModel = static_cast<GlossaryModel*>(m_proxyModel->sourceModel());

    if (i == -1) {
        //NOTE actually we should remove selected items, not current one
        const QModelIndex& current = m_browser->currentIndex();
        if (!current.isValid())
            return;
        i = m_proxyModel->mapToSource(current).row();
    }

    sourceModel->removeRow(i);
}

void GlossaryWindow::restore()
{
    setCaption(i18nc("@title:window", "Glossary"), false);

    Glossary* glossary = Project::instance()->glossary();
    glossary->load(glossary->path());
    m_reactOnSignals = false;
    showEntryInEditor(m_id);
    m_reactOnSignals = true;
}

bool GlossaryWindow::save()
{
    //TODO add error message
    return Project::instance()->glossary()->save();
}

bool GlossaryWindow::queryClose()
{
    Glossary* glossary = Project::instance()->glossary();

    applyEntryChange();
    if (glossary->isClean())
        return true;

    switch (KMessageBox::warningYesNoCancel(this,
                                            i18nc("@info", "The glossary contains unsaved changes.\n\
Do you want to save your changes or discard them?"), i18nc("@title:window", "Warning"),
                                            KStandardGuiItem::save(), KStandardGuiItem::discard())) {
    case KMessageBox::Yes:
        return save();
    case KMessageBox::No:
        restore();
        return true;
    default:
        return false;
    }
}


//END GlossaryWindow

void TermsListModel::setEntry(const QByteArray& id)
{
    m_id = id;
    QStringList terms = m_glossary->terms(m_id, m_lang);
    terms.append(QString()); //allow adding new terms
    setStringList(terms);
}

bool TermsListModel::setData(const QModelIndex& index, const QVariant& value, int role)
{
    Q_UNUSED(role);
    m_glossary->setTerm(m_id, m_lang, index.row(), value.toString());
    setEntry(m_id); //allow adding new terms
    return true;
}


bool TermsListModel::removeRows(int row, int count, const QModelIndex& parent)
{
    Q_UNUSED(count)
    if (row == rowCount() - 1)
        return false;// cannot delete non-existing item

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
    for (const QModelIndex& row : rows)
        model()->removeRow(row.row());
}


