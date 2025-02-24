/*
  This file is part of Lokalize

  SPDX-FileCopyrightText: 2009-2014 Nick Shaforostoff <shafff@ukr.net>
  SPDX-FileCopyrightText: 2018-2019 Simon Depiets <sdepiets@gmail.com>

  SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#include "phaseswindow.h"
#include "catalog.h"
#include "cmd.h"
#include "noteeditor.h"
#include "project.h"

#include <kcombobox.h>
#include <klocalizedstring.h>
#include <kstandardguiitem.h>

#include <QAbstractListModel>
#include <QApplication>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QPushButton>
#include <QSplitter>
#include <QStackedLayout>
#include <QStringBuilder>
#include <QStringListModel>
#include <QTextBrowser>
#include <QVBoxLayout>

// BEGIN PhasesModel
class PhasesModel : public QAbstractListModel
{
public:
    enum PhasesModelColumns {
        Date = 0,
        Process,
        Company,
        Contact,
        ToolName,
        ColumnCount,
    };

    PhasesModel(Catalog *catalog, QObject *parent);
    ~PhasesModel()
    {
    }
    QModelIndex addPhase(const Phase &phase);
    QModelIndex activePhaseIndex() const
    {
        return index(m_activePhase);
    }
    QList<Phase> addedPhases() const;

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override
    {
        Q_UNUSED(parent);
        return ColumnCount;
    }
    QVariant data(const QModelIndex &, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation, int role = Qt::DisplayRole) const override;

private:
    Catalog *m_catalog;
    QList<Phase> m_phases;
    QMap<QString, Tool> m_tools;
    int m_activePhase;
};

PhasesModel::PhasesModel(Catalog *catalog, QObject *parent)
    : QAbstractListModel(parent)
    , m_catalog(catalog)
    , m_phases(catalog->allPhases())
    , m_tools(catalog->allTools())
{
    m_activePhase = m_phases.size();
    while (--m_activePhase >= 0 && m_phases.at(m_activePhase).name != catalog->activePhase())
        ;
}

QModelIndex PhasesModel::addPhase(const Phase &phase)
{
    m_activePhase = m_phases.size();
    beginInsertRows(QModelIndex(), m_activePhase, m_activePhase);
    m_phases.append(phase);
    endInsertRows();
    return index(m_activePhase);
}

QList<Phase> PhasesModel::addedPhases() const
{
    QList<Phase> result;
    for (int i = m_catalog->allPhases().size(); i < m_phases.size(); ++i)
        result.append(m_phases.at(i));

    return result;
}

int PhasesModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    return m_phases.size();
}

QVariant PhasesModel::data(const QModelIndex &index, int role) const
{
    if (role == Qt::FontRole && index.row() == m_activePhase) {
        QFont font = QApplication::font();
        font.setBold(true);
        return font;
    }
    if (role == Qt::UserRole)
        return m_phases.at(index.row()).name;
    if (role != Qt::DisplayRole)
        return QVariant();

    const Phase &phase = m_phases.at(index.row());
    switch (index.column()) {
    case Date:
        return phase.date.toString();
    case Process:
        return phase.process;
    case Company:
        return phase.company;
    case Contact:
        return QString(phase.contact % (phase.email.isEmpty() ? QString() : QStringLiteral(" <%1> ").arg(phase.email))
                       % (phase.phone.isEmpty() ? QString() : QStringLiteral(", %1").arg(phase.phone)));
    case ToolName:
        return m_tools.value(phase.tool).name;
    }
    return QVariant();
}

QVariant PhasesModel::headerData(int section, Qt::Orientation, int role) const
{
    if (role != Qt::DisplayRole)
        return QVariant();

    switch (section) {
    case Date:
        return i18nc("@title:column", "Date");
    case Process:
        return i18nc("@title:column", "Process");
    case Company:
        return i18nc("@title:column", "Company");
    case Contact:
        return i18nc("@title:column", "Person");
    case ToolName:
        return i18nc("@title:column", "Tool");
    }
    return QVariant();
}
// END PhasesModel

// BEGIN PhaseEditDialog
class PhaseEditDialog : public QDialog
{
public:
    explicit PhaseEditDialog(QWidget *parent);
    ~PhaseEditDialog() = default;

    Phase phase() const;
    ProjectLocal::PersonRole role() const;

private:
    KComboBox *m_process{nullptr};
};

PhaseEditDialog::PhaseEditDialog(QWidget *parent)
    : QDialog(parent)
    , m_process(new KComboBox(this))
{
    QStringList processes;
    processes << i18n("Translation") << i18n("Review") << i18n("Approval");
    m_process->setModel(new QStringListModel(processes, this));

    QFormLayout *l = new QFormLayout(this);
    l->addRow(i18nc("noun", "Process (this will also change your role):"), m_process);

    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &PhaseEditDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &PhaseEditDialog::reject);
    l->addRow(buttonBox);
}

Phase PhaseEditDialog::phase() const
{
    Phase phase;
    phase.process = QLatin1String(processes()[m_process->currentIndex()]);
    return phase;
}

ProjectLocal::PersonRole PhaseEditDialog::role() const
{
    return (ProjectLocal::PersonRole)m_process->currentIndex();
}

PhasesWindow::PhasesWindow(Catalog *catalog, QWidget *parent)
    : QDialog(parent)
    , m_catalog(catalog)
    , m_model(new PhasesModel(catalog, this))
    , m_view(new MyTreeView(this))
    , m_browser(new QTextBrowser(this))
{
    connect(this, &PhasesWindow::accepted, this, &PhasesWindow::handleResult);
    QVBoxLayout *l = new QVBoxLayout(this);
    QHBoxLayout *btns = new QHBoxLayout;
    l->addLayout(btns);

    QPushButton *add = new QPushButton(this);
    KGuiItem::assign(add, KStandardGuiItem::add());

    connect(add, &QPushButton::clicked, this, &PhasesWindow::addPhase);
    btns->addWidget(add);
    btns->addStretch(5);

    QSplitter *splitter = new QSplitter(this);
    l->addWidget(splitter);

    m_buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(m_buttonBox, &QDialogButtonBox::accepted, this, &PhasesWindow::accept);
    connect(m_buttonBox, &QDialogButtonBox::rejected, this, &PhasesWindow::reject);
    l->addWidget(m_buttonBox);

    m_view->setRootIsDecorated(false);
    m_view->setModel(m_model);
    splitter->addWidget(m_view);
    int column = m_model->columnCount();
    while (--column >= 0)
        m_view->resizeColumnToContents(column);
    if (m_model->rowCount())
        m_view->setCurrentIndex(m_model->activePhaseIndex());
    connect(m_view, &MyTreeView::currentIndexChanged, this, &PhasesWindow::displayPhaseNotes);

    m_noteView = new QWidget(this);
    m_noteView->hide();
    splitter->addWidget(m_noteView);
    m_stackedLayout = new QStackedLayout(m_noteView);
    m_stackedLayout->addWidget(m_browser);

    m_browser->viewport()->setBackgroundRole(QPalette::Window);
    m_browser->setOpenLinks(false);
    connect(m_browser, &QTextBrowser::anchorClicked, this, &PhasesWindow::anchorClicked);

    splitter->setStretchFactor(0, 15);
    splitter->setStretchFactor(1, 5);
    resize(QSize(700, 400));
}

void PhasesWindow::handleResult()
{
    m_catalog->beginMacro(i18nc("@item Undo action item", "Edit phases"));

    Phase last;
    const auto addedPhases = m_model->addedPhases();
    for (const Phase &phase : addedPhases)
        static_cast<QUndoStack *>(m_catalog)->push(new UpdatePhaseCmd(m_catalog, last = phase));
    Project::instance()->local()->setRole(roleForProcess(last.process));
    m_catalog->setActivePhase(last.name, roleForProcess(last.process));

    QMapIterator<QString, QVector<Note>> i(m_phaseNotes);
    while (i.hasNext()) {
        i.next();
        m_catalog->setPhaseNotes(i.key(), i.value());
    }

    m_catalog->endMacro();
}

void PhasesWindow::addPhase()
{
    PhaseEditDialog d(this);
    if (!d.exec())
        return;

    Phase phase = d.phase();
    initPhaseForCatalog(m_catalog, phase, ForceAdd);
    m_view->setCurrentIndex(m_model->addPhase(phase));
    m_phaseNotes.insert(phase.name, QVector<Note>());

    m_buttonBox->button(QDialogButtonBox::Ok)->setFocus();
}

static QString phaseNameFromView(QTreeView *view)
{
    return view->currentIndex().data(Qt::UserRole).toString();
}

void PhasesWindow::anchorClicked(QUrl link)
{
    QString path = link.path().mid(1); // minus '/'

    if (link.scheme() == QLatin1String("note")) {
        if (!m_editor) {
            m_editor = new NoteEditor(this);
            m_stackedLayout->addWidget(m_editor);
            connect(m_editor, &NoteEditor::accepted, this, &PhasesWindow::noteEditAccepted);
            connect(m_editor, &NoteEditor::rejected, this, &PhasesWindow::noteEditRejected);
        }
        m_editor->setNoteAuthors(m_catalog->noteAuthors());
        if (path.endsWith(QLatin1String("add")))
            m_editor->setNote(Note(), -1);
        else {
            int pos = path.toInt();
            QString phaseName = phaseNameFromView(m_view);
            QVector<Note> notes = m_phaseNotes.contains(phaseName) ? m_phaseNotes.value(phaseName) : m_catalog->phaseNotes(phaseName);
            m_editor->setNote(notes.at(pos), pos);
        }
        m_stackedLayout->setCurrentIndex(1);
    }
}

void PhasesWindow::noteEditAccepted()
{
    QString phaseName = phaseNameFromView(m_view);
    if (!m_phaseNotes.contains(phaseName))
        m_phaseNotes.insert(phaseName, m_catalog->phaseNotes(phaseName));

    if (m_editor->noteIndex() == -1)
        m_phaseNotes[phaseName].append(m_editor->note());
    else
        m_phaseNotes[phaseName][m_editor->noteIndex()] = m_editor->note();

    m_stackedLayout->setCurrentIndex(0);
    displayPhaseNotes(m_view->currentIndex());
}

void PhasesWindow::noteEditRejected()
{
    m_stackedLayout->setCurrentIndex(0);
}

void PhasesWindow::displayPhaseNotes(const QModelIndex &current)
{
    m_browser->clear();
    QString phaseName = current.data(Qt::UserRole).toString();
    QVector<Note> notes = m_phaseNotes.contains(phaseName) ? m_phaseNotes.value(phaseName) : m_catalog->phaseNotes(phaseName);
    displayNotes(m_browser, notes);
    m_noteView->show();
    m_stackedLayout->setCurrentIndex(0);
}

#include "moc_phaseswindow.cpp"
