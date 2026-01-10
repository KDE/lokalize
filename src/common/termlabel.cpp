/*
  This file is part of Lokalize

  SPDX-FileCopyrightText: 2007      Nick Shaforostoff <shafff@ukr.net>
  SPDX-FileCopyrightText: 2018-2019 Simon Depiets <sdepiets@gmail.com>
  SPDX-FileCopyrightText: 2025      Finley Watson <fin-w@tutanota.com>

  SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#include "termlabel.h"
#include "glossary.h"
#include "glossarytab.h"
#include "project.h"

#include <KLocalizedString>

#include <QMenu>
#include <QMouseEvent>
#include <QStringBuilder>

using namespace GlossaryNS;

void TermLabel::insert()
{
    GlossaryNS::Glossary *glossary = Project::instance()->glossary();
    if (m_entryId.isEmpty())
        return;
    QString termTrans;
    const QStringList &termTarget = glossary->terms(m_entryId, Project::instance()->targetLangCode());
    if (termTarget.count() > 1) {
        QMenu menu;

        int limit = termTarget.count();
        menu.setActiveAction(menu.addAction(termTarget.at(0)));
        int i = 1;
        for (; i < limit; ++i)
            menu.addAction(termTarget.at(i));

        QAction *txt = menu.exec(mapToGlobal(QPoint(0, 0)));
        if (!txt)
            return;
        termTrans = txt->text();
    } else if (termTarget.count() == 1)
        termTrans = termTarget.first();

    if (m_capFirst && !termTrans.isEmpty())
        termTrans[0] = termTrans.at(0).toUpper();

    Q_EMIT insertTerm(termTrans);
}

void TermLabel::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::RightButton) {
        QMenu menu;

        menu.addAction(i18nc("@action:inmenu Edit term", "Edit"));

        QAction *txt = menu.exec(event->globalPosition().toPoint());
        if (txt) {
            GlossaryNS::GlossaryTab *glossaryTab = Project::instance()->glossaryTab();
            if (glossaryTab)
                glossaryTab->selectEntry(m_entryId);
        }
    } else
        insert();
}

void TermLabel::setText(const QString &term, const QByteArray &entryId, bool capFirst)
{
    m_entryId = entryId;
    m_capFirst = capFirst;

    static const QString n = QStringLiteral("  \n  ");
    QLabel::setText(
        QString(term
                + QString(m_action ? QString(QStringLiteral(" [") + m_action->shortcut().toString(QKeySequence::NativeText) + QStringLiteral("]  \n  "))
                                   : n) // m_shortcut
                + Project::instance()->glossary()->terms(m_entryId, Project::instance()->targetLangCode()).join(n) + n));
}

#include "moc_termlabel.cpp"
