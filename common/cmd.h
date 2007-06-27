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



#ifndef CMD_H
#define CMD_H

#include <QUndoCommand>
#include <kdebug.h>

#include "pos.h"
#include "catalog.h"
#include "catalog_private.h"
#include "catalogitem_private.h"

enum Commands { Insert, Delete, ToggleFuzzy };

class InsTextCmd : public QUndoCommand
{
public:
    InsTextCmd(Catalog *catalog,const DocPosition &pos,const QString &str);
    virtual ~InsTextCmd(){};
    int id () const {return Insert;}
    bool mergeWith(const QUndoCommand *other);
    void undo();
    void redo();
private:
    Catalog* _catalog;
    QString _str;
    DocPosition _pos;
};


class DelTextCmd : public QUndoCommand
{
public:
    DelTextCmd(Catalog *catalog,const DocPosition &pos,const QString &str);
    virtual ~DelTextCmd(){};
    int id () const {return Delete;}
    bool mergeWith(const QUndoCommand *other);
    void redo();
    void undo();
private:
    Catalog* _catalog;
    QString _str;
    DocPosition _pos;
};

/* you should care not to new it w/ aint no need */
class ToggleFuzzyCmd : public QUndoCommand
{
public:
    ToggleFuzzyCmd(Catalog *catalog,uint index,bool flag);
    virtual ~ToggleFuzzyCmd(){};
    int id () const {return ToggleFuzzy;}
    void redo();
    void undo();
private:
    void unsetFuzzy();
    void setFuzzy();

    Catalog* _catalog;
    bool _flag;
    uint _index;
};


#endif // CMD_H
