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

#ifndef MULTIEDITORADAPTOR_H
#define MULTIEDITORADAPTOR_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "editoradaptor.h"
#include <kdebug.h>

/**
 * Hack over QDBusAbstractAdaptor to get kross active-editor-adaptor for free
 */
class MultiEditorAdaptor: public EditorAdaptor
{
    Q_OBJECT
public:
    MultiEditorAdaptor(EditorTab *parent);
    ~MultiEditorAdaptor() { /*kWarning()<<"bye bye cruel world";*/ }

    inline EditorTab* editorTab() const
    { return static_cast<EditorTab*>(QObject::parent()); }

    void setEditorTab(EditorTab* e);

private slots:
    void handleParentDestroy(QObject* p);
};

//methosa are defined in lokalizemainwindow.cpp


#endif
