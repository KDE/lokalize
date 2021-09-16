/*
  This file is part of Lokalize

  SPDX-FileCopyrightText: 2007-2009 Nick Shaforostoff <shafff@ukr.net>
  SPDX-FileCopyrightText: 2018-2019 Simon Depiets <sdepiets@gmail.com>

  SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL

*/

#ifndef MULTIEDITORADAPTOR_H
#define MULTIEDITORADAPTOR_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "editoradaptor.h"

/**
 * Hack over QDBusAbstractAdaptor to get kross active-editor-adaptor for free
 */
class MultiEditorAdaptor: public EditorAdaptor
{
    Q_OBJECT
public:
    explicit MultiEditorAdaptor(EditorTab *parent);
    ~MultiEditorAdaptor()
    {
        /*qCWarning(LOKALIZE_LOG)<<"bye bye cruel world";*/
    }

    inline EditorTab* editorTab() const
    {
        return static_cast<EditorTab*>(QObject::parent());
    }

    void setEditorTab(EditorTab* e);

private Q_SLOTS:
    void handleParentDestroy(QObject* p);
};

//methosa are defined in lokalizemainwindow.cpp


#endif
