/* ****************************************************************************
  This file is part of Lokalize

  Copyright (C) 2014 by Nick Shaforostoff <shafff@ukr.net>

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

#ifndef WELCOMETAB_H
#define WELCOMETAB_H

#include "lokalizesubwindowbase.h"
#include "ui_welcomewidget.h"

class WelcomeTab: public LokalizeSubwindowBase2, public Ui_WelcomeWidget
{
    Q_OBJECT

public:
    WelcomeTab(QWidget* parent);
    ~WelcomeTab();

protected:
    virtual void dragEnterEvent(QDragEnterEvent*);
    virtual void dropEvent(QDropEvent*);

};

//helps translate 'Russian (ru_UA)' -> 'ru_UA'
class LangCodeSaver: public QObject
{
Q_OBJECT
public:
    LangCodeSaver(QWidget* p):QObject(p){}
public slots:
    void setLangCode(int index);
signals:
    void langCodeSelected(const QString&);
};


#endif
