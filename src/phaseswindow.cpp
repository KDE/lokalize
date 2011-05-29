/* ****************************************************************************
  This file is part of Lokalize

  Copyright (C) 2009 by Nick Shaforostoff <shafff@ukr.net>

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

#include "phaseswindow.h"
#include "catalog.h"
#include "cmd.h"
#include "noteeditor.h"

#include <klocale.h>
#include <KPushButton>
#include <KTextBrowser>
#include <KComboBox>

#include <QTreeView>
#include <QStringListModel>
#include <QVBoxLayout>
#include <QFormLayout>
#include <QApplication>


//BEGIN PhasesModel
#include <QAbstractListModel>
#include <QSplitter>
#include <QStackedLayout>

class PhasesModel: public QAbstractListModel
{
public:
    enum PhasesModelColumns
    {
        Date=0,
        Process,
        Company,
        Contact,
        ToolName,
        ColumnCount
    };

    PhasesModel(Catalog* catalog, QObject* parent);
    ~PhasesModel(){}
    QModelIndex addPhase(const Phase& phase);
    QModelIndex activePhaseIndex()const{return index(m_activePhase);}
    QList<Phase> addedPhases()const;

    int rowCount(const QModelIndex& parent=QModelIndex()) const;
    int columnCount(const QModelIndex& parent=QModelIndex()) const{Q_UNUSED(parent); return ColumnCount;}
    QVariant data(const QModelIndex&,int role=Qt::DisplayRole) const;
    QVariant headerData(int section, Qt::Orientation, int role=Qt::DisplayRole) const;


private:
    Catalog* m_catalog;
    QList<Phase> m_phases;
    QMap<QString,Tool> m_tools;
    int m_activePhase;
};

PhasesModel::PhasesModel(Catalog* catalog, QObject* parent)
    : QAbstractListModel(parent)
    , m_catalog(catalog)
    , m_phases(catalog->allPhases())
    , m_tools(catalog->allTools())
{
    m_activePhase=m_phases.size();
    while (--m_activePhase>=0 && m_phases.at(m_activePhase).name!=catalog->activePhase())
        ;
}

QModelIndex PhasesModel::addPhase(const Phase& phase)
{
    m_activePhase=m_phases.size();
    beginInsertRows(QModelIndex(),m_activePhase,m_activePhase);
    m_phases.append(phase);
    endInsertRows();
    return index(m_activePhase);
}

QList<Phase> PhasesModel::addedPhases()const
{
    QList<Phase> result;
    for (int i=m_catalog->allPhases().size();i<m_phases.size();++i)
        result.append(m_phases.at(i));

    return result;
}

int PhasesModel::rowCount(const QModelIndex& parent) const
{
    if (parent.isValid())
        return 0;
    return m_phases.size();
}

QVariant PhasesModel::data(const QModelIndex& index, int role) const
{
    if (role==Qt::FontRole && index.row()==m_activePhase)
    {
        QFont font=QApplication::font();
        font.setBold(true);
        return font;
    }
    if (role==Qt::UserRole)
        return m_phases.at(index.row()).name;
    if (role!=Qt::DisplayRole)
        return QVariant();

    const Phase& phase=m_phases.at(index.row());
    switch (index.column())
    {
        case Date:       return phase.date.toString();
        case Process:    return phase.process;
        case Company:    return phase.company;
        case Contact:    return QString(phase.contact
                           +(phase.email.isEmpty()?"":QString(" <%1> ").arg(phase.email))
                           +(phase.phone.isEmpty()?"":QString(", %1").arg(phase.phone)));
        case ToolName:       return m_tools.value(phase.tool).name;
    }
    return QVariant();
}

QVariant PhasesModel::headerData(int section, Qt::Orientation, int role) const
{
    if (role!=Qt::DisplayRole)
        return QVariant();

    switch (section)
    {
        case Date:       return i18nc("@title:column","Date");
        case Process:    return i18nc("@title:column","Process");
        case Company:    return i18nc("@title:column","Company");
        case Contact:    return i18nc("@title:column","Person");
        case ToolName:   return i18nc("@title:column","Tool");
    }
    return QVariant();
}
//END PhasesModel


//BEGIN PhaseEditDialog
class PhaseEditDialog: public KDialog
{
public:
    PhaseEditDialog(QWidget *parent);
    ~PhaseEditDialog(){}

    Phase phase()const;
private:
    KComboBox* m_process;
};


PhaseEditDialog::PhaseEditDialog(QWidget *parent)
    : KDialog(parent)
    , m_process(new KComboBox(this))
{
    QStringList processes;
    processes<<i18n("Translation")<<i18n("Review")<<i18n("Approval");
    m_process->setModel(new QStringListModel(processes,this));

    QFormLayout* l=new QFormLayout(mainWidget());
    l->addRow(i18n("Process"), m_process);
}

Phase PhaseEditDialog::phase() const
{
    Phase phase;
    phase.process=processes()[m_process->currentIndex()];
    return phase;
}

PhasesWindow::PhasesWindow(Catalog* catalog, QWidget *parent)
 : KDialog(parent)
 , m_catalog(catalog)
 , m_model(new PhasesModel(catalog, this))
 , m_view(new MyTreeView(this))
 , m_browser(new KTextBrowser(this))
 , m_editor(0)
{
    connect(this, SIGNAL(accepted()), SLOT(handleResult()));
    //setAttribute(Qt::WA_DeleteOnClose, true);
    QVBoxLayout* l=new QVBoxLayout(mainWidget());
    QHBoxLayout* btns=new QHBoxLayout;
    l->addLayout(btns);

    KPushButton* add=new KPushButton(KStandardGuiItem::add(),this);
    connect(add, SIGNAL(clicked()),this,SLOT(addPhase()));
    btns->addWidget(add);
    btns->addStretch(5);
/*
    KPushButton* add=new KPushButton(i18nc("@action:button",""),this);
    btns->addWidget(activate);
*/
    QSplitter* splitter=new QSplitter(this);
    l->addWidget(splitter);

    m_view->setRootIsDecorated(false);
    m_view->setModel(m_model);
    splitter->addWidget(m_view);
    int column=m_model->columnCount();
    while (--column>=0)
        m_view->resizeColumnToContents(column);
    if (m_model->rowCount())
        m_view->setCurrentIndex(m_model->activePhaseIndex());
    connect(m_view, SIGNAL(currentIndexChanged(QModelIndex)), SLOT(displayPhaseNotes(QModelIndex)));


    m_noteView=new QWidget(this);
    m_noteView->hide();
    splitter->addWidget(m_noteView);
    m_stackedLayout = new QStackedLayout(m_noteView);
    m_stackedLayout->addWidget(m_browser);

    m_browser->viewport()->setBackgroundRole(QPalette::Background);
    m_browser->setOpenLinks(false);
    connect(m_browser,SIGNAL(anchorClicked(QUrl)),this,SLOT(anchorClicked(QUrl)));

    splitter->setStretchFactor(0,15);
    splitter->setStretchFactor(1,5);
    setInitialSize(QSize(700,400));
}

void PhasesWindow::handleResult()
{
    m_catalog->beginMacro(i18nc("@item Undo action item", "Edit phases"));

    Phase last;
    foreach(const Phase& phase, m_model->addedPhases())
        static_cast<QUndoStack*>(m_catalog)->push(new UpdatePhaseCmd(m_catalog, last=phase));
    m_catalog->setActivePhase(last.name,roleForProcess(last.process));

    QMapIterator<QString, QVector<Note> > i(m_phaseNotes);
    while (i.hasNext())
    {
        i.next();
        m_catalog->setPhaseNotes(i.key(),i.value());
    }

    m_catalog->endMacro();
}

void PhasesWindow::addPhase()
{
    PhaseEditDialog d(this);
    if (!d.exec())
        return;

    Phase phase=d.phase();
    initPhaseForCatalog(m_catalog, phase, ForceAdd);
    m_view->setCurrentIndex(m_model->addPhase(phase));
    m_phaseNotes.insert(phase.name, QVector<Note>());
}

static QString phaseNameFromView(QTreeView* view)
{
    return view->currentIndex().data(Qt::UserRole).toString();
}

void PhasesWindow::anchorClicked(QUrl link)
{
    QString path=link.path().mid(1);// minus '/'

    if (link.scheme()=="note")
    {
        if (!m_editor)
        {
            m_editor=new NoteEditor(this);
            m_stackedLayout->addWidget(m_editor);
            connect(m_editor,SIGNAL(accepted()),this,SLOT(noteEditAccepted()));
            connect(m_editor,SIGNAL(rejected()),this,SLOT(noteEditRejected()));
        }
        m_editor->setNoteAuthors(m_catalog->noteAuthors());
        if (path.endsWith("add"))
            m_editor->setNote(Note(),-1);
        else
        {
            int pos=path.toInt();
            QString phaseName=phaseNameFromView(m_view);
            QVector<Note> notes=m_phaseNotes.contains(phaseName)?
                                m_phaseNotes.value(phaseName)
                                :m_catalog->phaseNotes(phaseName);
            m_editor->setNote(notes.at(pos),pos);
        }
        m_stackedLayout->setCurrentIndex(1);
    }
}

void PhasesWindow::noteEditAccepted()
{
    QString phaseName=phaseNameFromView(m_view);
    if (!m_phaseNotes.contains(phaseName))
        m_phaseNotes.insert(phaseName, m_catalog->phaseNotes(phaseName));

    QVector<Note> notes=m_phaseNotes.value(phaseName);
    if (m_editor->noteIndex()==-1)
        m_phaseNotes[phaseName].append(m_editor->note());
    else
        m_phaseNotes[phaseName][m_editor->noteIndex()]=m_editor->note();

    m_stackedLayout->setCurrentIndex(0);
    displayPhaseNotes(m_view->currentIndex());
}

void PhasesWindow::noteEditRejected()
{
    m_stackedLayout->setCurrentIndex(0);
}

void PhasesWindow::displayPhaseNotes(const QModelIndex& current)
{
    m_browser->clear();
    QString phaseName=current.data(Qt::UserRole).toString();
    QVector<Note> notes=m_phaseNotes.contains(phaseName)?
                        m_phaseNotes.value(phaseName)
                        :m_catalog->phaseNotes(phaseName);
    kWarning()<<notes.size();
    displayNotes(m_browser, notes);
    m_noteView->show();
    m_stackedLayout->setCurrentIndex(0);
}

