/* ****************************************************************************
  This file is part of Lokalize

  Copyright (C) 2007-2009 by Nick Shaforostoff <shafff@ukr.net>

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
#include "glossary.h"
#include "project.h"

#include "ui_termedit.h"

#include <kdebug.h>
#include <klineedit.h>
#include <kpushbutton.h>
#include <kguiitem.h>
#include <kmessagebox.h>


#include <QSplitter>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSortFilterProxyModel>
#include <QAbstractItemModel>

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

void GlossaryTreeView::currentChanged(const QModelIndex& current,const QModelIndex&/* previous*/)
{
    if (current.isValid())
        emit currentChanged(static_cast<QSortFilterProxyModel*>(model())->mapToSource(current).row());
}

void GlossaryTreeView::selectRow(int i)
{
    QSortFilterProxyModel* proxyModel=static_cast<QSortFilterProxyModel*>(model());
    GlossaryModel* sourceModel=static_cast<GlossaryModel*>(proxyModel->sourceModel());

    //sourceModel->forceReset();
    setCurrentIndex(proxyModel->mapFromSource(sourceModel->index(i,0)));
}


//END GlossaryTreeView


//BEGIN SubjectFieldModel

class SubjectFieldModel: public QAbstractItemModel
{
public:

    //Q_OBJECT

    SubjectFieldModel(QObject* parent);
    ~SubjectFieldModel(){}

    QModelIndex index (int row, int column, const QModelIndex& parent=QModelIndex()) const;
    QModelIndex parent(const QModelIndex&) const;
    int rowCount(const QModelIndex& parent=QModelIndex()) const;
    int columnCount(const QModelIndex& parent=QModelIndex()) const;
    QVariant data(const QModelIndex&,int role=Qt::DisplayRole) const;
    bool setData (const QModelIndex&,const QVariant&,int role=Qt::EditRole);
    bool setItemData(const QModelIndex& index, const QMap<int,QVariant>& roles);
    bool insertRows(int row, int count, const QModelIndex& parent=QModelIndex());
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

QModelIndex SubjectFieldModel::index (int row,int column,const QModelIndex& /*parent*/) const
{
    return createIndex (row, column);
}

Qt::ItemFlags SubjectFieldModel::flags(const QModelIndex&) const
{
    return Qt::ItemIsSelectable|Qt::ItemIsEditable|Qt::ItemIsEnabled;
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

QVariant SubjectFieldModel::data(const QModelIndex& index,int role) const
{
    if (role==Qt::DisplayRole||role==Qt::EditRole)
        return Project::instance()->glossary()->subjectFields.at(index.row());
    return QVariant();
}

bool SubjectFieldModel::insertRows(int row, int count, const QModelIndex& parent)
{
    beginInsertRows(parent,row,row+count-1);

    QStringList& subjectFields=Project::instance()->glossary()->subjectFields;

    while (--count>=0)
        subjectFields.insert(row+count,QString());

    endInsertRows();
    return true;
}

bool SubjectFieldModel::setData(const QModelIndex& index, const QVariant& value, int role)
{
    kDebug()<<role;
    QStringList& subjectFields=Project::instance()->glossary()->subjectFields;
    subjectFields[index.row()]=value.toString();
    return true;
}

bool SubjectFieldModel::setItemData(const QModelIndex& index, const QMap<int,QVariant>& roles)
{
    if (roles.contains(Qt::EditRole))
    {
        QStringList& subjectFields=Project::instance()->glossary()->subjectFields;
        subjectFields[index.row()]=roles.value(Qt::EditRole).toString();
    }
    return true;
}

//END SubjectFieldModel

//BEGIN GlossaryWindow

GlossaryWindow::GlossaryWindow(QWidget *parent)
 : KMainWindow(parent)
 , m_browser(new GlossaryTreeView(this))
 , m_proxyModel(new QSortFilterProxyModel(this))
 , m_reactOnSignals(true)
{
    setAttribute(Qt::WA_DeleteOnClose, true);

    QSplitter* splitter=new QSplitter(Qt::Horizontal,this);
    setCentralWidget(splitter);

    m_proxyModel->setFilterKeyColumn(GlossaryModel::English);
    GlossaryModel* gloModel=new GlossaryModel(this);
    m_proxyModel->setSourceModel(gloModel);
    m_browser->setModel(m_proxyModel);
    //m_browser->setColumnWidth(GlossaryModel::ID, m_browser->columnWidth(GlossaryModel::ID)/2); //man this is  HACK y
    m_browser->setColumnWidth(GlossaryModel::English, m_browser->columnWidth(GlossaryModel::English)*2); //man this is  HACK y
    m_browser->setColumnWidth(GlossaryModel::Target, m_browser->columnWidth(GlossaryModel::Target)*2);
    m_browser->setAlternatingRowColors(true);

    //left
    QWidget* w=new QWidget(splitter);
    QVBoxLayout* layout=new QVBoxLayout(w);
    m_lineEdit=new KLineEdit(w);
    m_lineEdit->setClearButtonShown(true);
    m_lineEdit->setFocus();
//     connect (m_lineEdit,SIGNAL(textChanged(QString)),
//              m_proxyModel,SLOT(setFilterFixedString(QString)));
    connect (m_lineEdit,SIGNAL(textChanged(QString)),
             m_proxyModel,SLOT(setFilterRegExp(QString)));

    layout->addWidget(m_lineEdit);
    layout->addWidget(m_browser);
    {
        KPushButton* addBtn=new KPushButton(KStandardGuiItem::add(),w);
        connect(addBtn,SIGNAL(clicked()),       this,SLOT(newTerm()));

        KPushButton* rmBtn=new KPushButton(KStandardGuiItem::remove(),w);
        connect(rmBtn,SIGNAL(clicked()),        this,SLOT(rmTerm()));

        KPushButton* restoreBtn=new KPushButton(i18nc("@action:button reloads glossary from disk","Restore"),w);
        restoreBtn->setToolTip(i18nc("@info:tooltip","Reload glossary from disk, discarding any changes"));
        connect(restoreBtn,SIGNAL(clicked()),   this,SLOT(restore()));

        QWidget* btns=new QWidget(w);
        QHBoxLayout* btnsLayout=new QHBoxLayout(btns);
        btnsLayout->addWidget(addBtn);
        btnsLayout->addWidget(rmBtn);
        btnsLayout->addWidget(restoreBtn);

        layout->addWidget(btns);
        //QWidget::setTabOrder(m_browser,addBtn);
        QWidget::setTabOrder(addBtn,rmBtn);
        QWidget::setTabOrder(rmBtn,restoreBtn);
        QWidget::setTabOrder(restoreBtn,m_lineEdit);
    }
    QWidget::setTabOrder(m_lineEdit,m_browser);

    splitter->addWidget(w);

    //right
    w=new QWidget(splitter);
    m_editor=w;
    w->hide();
    Ui_TermEdit ui_termEdit;
    ui_termEdit.setupUi(w);
    splitter->addWidget(w);

    m_english=ui_termEdit.english;
    m_target=ui_termEdit.target;
    m_subjectField=ui_termEdit.subjectField;
    m_definition=ui_termEdit.definition;

    connect (m_english,SIGNAL(textChanged()),   this,SLOT(chTerm()));
    connect (m_target,SIGNAL(textChanged()),    this,SLOT(chTerm()));
    connect (m_definition,SIGNAL(textChanged()),this,SLOT(chTerm()));
    connect (m_subjectField,SIGNAL(editTextChanged(QString)),this,SLOT(chTerm()));


    //m_subjectField->addItems(Project::instance()->glossary()->subjectFields);
    m_subjectField->setModel(new SubjectFieldModel(this));
    connect(m_browser,SIGNAL(currentChanged(int)), this,SLOT(currentChanged(int)));
    connect(m_browser,SIGNAL(clicked(QModelIndex)),m_english,SLOT(setFocus()));


    setAutoSaveSettings(QLatin1String("GlossaryWindow"),true);
    Glossary* glo=Project::instance()->glossary();
    setCaption(i18nc("@title:window","Glossary"),
              !glo->changedIds.isEmpty()||!glo->addedIds.isEmpty()||!glo->removedIds.isEmpty());
}


GlossaryWindow::~GlossaryWindow()
{
}

void GlossaryWindow::currentChanged(int i)
{
//    kDebug()<<"start"<<i;
    m_reactOnSignals=false;

    m_editor->show();

    const TermEntry& a=Project::instance()->glossary()->termList.at(i);
    m_english->setPlainText(a.english.join("\n"));
    m_target->setPlainText(a.target.join("\n"));
    m_subjectField->setCurrentIndex(a.subjectField);
    m_definition->setPlainText(a.definition);

    m_reactOnSignals=true;
    //kDebug()<<"end";
}

void GlossaryWindow::chTerm()
{
    if (!m_reactOnSignals)
        return;

    kDebug();
//  QSortFilterProxyModel* proxyModel=static_cast<QSortFilterProxyModel*>(model());
    //GlossaryModel* sourceModel=static_cast<GlossaryModel*>(m_proxyModel->sourceModel());
    const QModelIndex& idx=m_proxyModel->mapToSource( m_browser->currentIndex() );
    if (!idx.isValid())
        return;
    setCaption(i18nc("@title:window","Glossary"),true);

    int index=idx.row();
    Glossary* glo=Project::instance()->glossary();
    glo->unhashTermEntry(index);//we will rehash it after applying changes

    QString id(glo->termList.at(index).id);
    if (! (glo->changedIds.contains(id)||glo->addedIds.contains(id)) )
    {
        kDebug()<<"append";
        glo->changedIds.append(id);
    }

    TermEntry& a=glo->termList[index];
    a.english=m_english->toPlainText().split('\n');
    a.target=m_target->toPlainText().split('\n');
    a.definition=m_definition->toPlainText();
    a.subjectField=glo->subjectFields.indexOf(m_subjectField->currentText());
    if ((a.subjectField==-1) && !m_subjectField->currentText().isEmpty())
    {
        a.subjectField=glo->subjectFields.size();
        glo->subjectFields.append(m_subjectField->currentText());
    }

    glo->hashTermEntry(index);


    //update the GUI
    const QModelIndex& parent=idx.parent();
    int row=m_browser->currentIndex().row();
    int i=m_proxyModel->columnCount();
    while (--i>=0)
        m_browser->update(m_proxyModel->index(row,i,parent));

    kDebug()<<glo->changedIds;
    glo->forceChangeSignal();
}


void GlossaryWindow::newTerm(QString _english, QString _target)
{
//     kDebug()<<"start";
    setCaption(i18nc("@title:window","Glossary"),true);

    const Glossary* glo=Project::instance()->glossary();
    GlossaryModel* sourceModel=static_cast<GlossaryModel*>(m_proxyModel->sourceModel());
    if (sourceModel->appendRow(_english,_target))
        m_browser->selectRow(glo->termList.size()-1);
//     kDebug()<<"end";
    m_english->setFocus();
    kDebug()<<glo->addedIds;
}

void GlossaryWindow::selectTerm(int index)
{
    m_browser->selectRow(index);
}

void GlossaryWindow::rmTerm(int i)
{
    setCaption(i18nc("@title:window","Glossary"),true);

    //QSortFilterProxyModel* proxyModel=static_cast<QSortFilterProxyModel*>(model());
    GlossaryModel* sourceModel=static_cast<GlossaryModel*>(m_proxyModel->sourceModel());

    if (i==-1)
    {
        //NOTE actually we should remove selected items, not current one
        const QModelIndex& current=m_browser->currentIndex();
        if (!current.isValid())
            return;
        i=m_proxyModel->mapToSource(current).row();
    }

    sourceModel->removeRow(i);
    const Glossary* glo=Project::instance()->glossary();
    kDebug()<<glo->removedIds;
}

void GlossaryWindow::restore()
{
    setCaption(i18nc("@title:window","Glossary"),false);

    Glossary* glo=Project::instance()->glossary();
    glo->load(glo->path);
    GlossaryModel* sourceModel=static_cast<GlossaryModel*>(m_proxyModel->sourceModel());
    sourceModel->forceReset();
}


bool GlossaryWindow::queryClose()
{
    Glossary* glo=Project::instance()->glossary();

    if (glo->changedIds.isEmpty()
        &&glo->addedIds.isEmpty()
        &&glo->removedIds.isEmpty())
        return true;

    switch(KMessageBox::warningYesNoCancel(this,
        i18nc("@info","The glossary contains unsaved changes.\n\
Do you want to save your changes or discard them?"),i18nc("@title:window","Warning"),
      KStandardGuiItem::save(),KStandardGuiItem::discard()))
    {
        case KMessageBox::Yes:
            glo->save();
            return true;
        case KMessageBox::No:
            glo->load(glo->path);
            return true;
        default:
            return false;
    }
}


//END GlossaryWindow

#include "glossarywindow.moc"
