/* ****************************************************************************
  This file is part of Lokalize

  Copyright (C) 2008 by Nick Shaforostoff <shafff@ukr.net>

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

#ifndef LOKALIZESUBWINDOWBASE_H
#define LOKALIZESUBWINDOWBASE_H

#include <QHash>
#include <QString>
#include <KMainWindow>
#include <KXMLGUIClient>

#include <kurl.h>
#include "actionproxy.h"


/**
 * Interface for LokalizeMainWindow
 */
class LokalizeSubwindowBase: public KMainWindow
{
Q_OBJECT
public:
    LokalizeSubwindowBase(QWidget* parent):KMainWindow(parent){}
    virtual ~LokalizeSubwindowBase(){emit aboutToBeClosed();}
    virtual KXMLGUIClient* guiClient()=0;

    //interface for LokalizeMainWindow
    virtual void hideDocks()=0;
    virtual void showDocks()=0;
    //bool queryClose();

    virtual KUrl currentUrl(){return KUrl();}

protected:
    void reflectNonApprovedCount(int count, int total);
    void reflectUntranslatedCount(int count, int total);

signals:
    void aboutToBeClosed();

public:
    //QHash<QString,ActionProxy*> supportedActions;
    StatusBarProxy statusBarItems;

};

/**
 * C++ casting workaround
 */
class LokalizeSubwindowBase2: public LokalizeSubwindowBase, public KXMLGUIClient
{
public:
    LokalizeSubwindowBase2(QWidget* parent): LokalizeSubwindowBase(parent),KXMLGUIClient(){}
    virtual ~LokalizeSubwindowBase2(){};

    KXMLGUIClient* guiClient(){return (KXMLGUIClient*)this;}
};


#endif
