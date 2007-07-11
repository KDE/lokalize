/* ****************************************************************************
  This file is part of KAider

  Copyright (C) 2007 by Nick Shaforostoff <shafff@ukr.net>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

  In addition, as a special exception, the copyright holders give
  permission to link the code of this program with any edition of
  the Qt library by Trolltech AS, Norway (or with modified versions
  of Qt that use the same license as Qt), and distribute linked
  combinations including the two.  You must obey the GNU General
  Public License in all respects for all of the code used other than
  Qt. If you modify this file, you may extend this exception to
  your version of the file, but you are not obligated to do so.  If
  you do not wish to do so, delete this exception statement from
  your version.

**************************************************************************** */

#ifndef TERMLABEL_H
#define TERMLABEL_H

#include <QLabel>
#include <QAction>
#include "glossary.h"
#include "project.h"
//class QAction;
// #include <QPushButton>

/**
 * flowlayout item
 */
class TermLabel: public QLabel//QPushButton
{
    Q_OBJECT
public:
    TermLabel(QAction* a=0): m_action(a){};
    ~TermLabel(){}

    /**
     * @param term is the term matched
     * @param entry is a whole entry
     */
    void setText(const QString& term,int entry);
    void mousePressEvent (QMouseEvent*/* event*/);

public slots:
    void insert();
//     bool event(QEvent *event);
signals:
    void insertTerm(const QString&);

private:
    int m_termIndex;
    QAction* m_action; //used only for shortcut purposes
};




inline
void TermLabel::setText(const QString& term,int entry)
{
    m_termIndex=entry;
    QLabel::setText(term + QString(m_action?(" [" + m_action->shortcut().toString()+"]  \n  "):"  \n  ")//m_shortcut
                + Project::instance()->glossary()->termList.at(m_termIndex).target.join("  \n  ")
                    + "  \n  ");
}






/**
 * flowlayout item
 */
class QueryResultBtn: public QLabel
{
    Q_OBJECT
public:
    QueryResultBtn(QAction* a=0);
    virtual ~QueryResultBtn(){}

    void mousePressEvent (QMouseEvent*/* event*/);
    void setText(const QString&);
public slots:
    void insert();
//     bool event(QEvent *event);
signals:
    void insertText(const QString&);

private:
    QString m_text;
    QAction* m_action;
};



inline
void QueryResultBtn::setText(const QString& queryResult)
{
    m_text=queryResult;
    if (m_action)
        QLabel::setText("["+m_action->shortcut().toString()+"] "+queryResult);
    else
        QLabel::setText(queryResult);
}





#endif
