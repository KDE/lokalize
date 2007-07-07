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



#include <QString>

#include <klocale.h>
#include <kdebug.h>

// #include <kmessagebox.h>
// KMessageBox::information(0, QString("'%1'").arg(_str));

// #include "global.h"
#include "pos.h"
#include "cmd.h"
#include "catalog_private.h"
#include "catalogitem_private.h"

//#define ITEM Catalog::instance()->d->_entries.at(_pos.entry).d
#define ITEM _catalog->d->_entries.at(_pos.entry).d

InsTextCmd::InsTextCmd(Catalog *catalog,const DocPosition &pos,const QString &str)
    : QUndoCommand(i18nc("Undo command","Insertion"))
    , _catalog(catalog)
    , _str(str)
    , _pos(pos)
{
}

bool InsTextCmd::mergeWith(const QUndoCommand *other)
{
    if (
        (other->id() != id())
        || (static_cast<const InsTextCmd*>(other)->_pos.entry!=_pos.entry)
        || (static_cast<const InsTextCmd*>(other)->_pos.form!=_pos.form)
        || (static_cast<const InsTextCmd*>(other)->_pos.offset!=_pos.offset+_str.size())
        )
        return false;
    _str += static_cast<const InsTextCmd*>(other)->_str;
    return true;
}

void InsTextCmd::redo()
{
    if ((!_pos.offset)&&(ITEM->_msgstrPlural[_pos.form].isEmpty()))
    {
        _catalog->d->_untransIndex.removeAll(_pos.entry);
        _catalog->emitsignalNumberOfUntranslatedChanged();
    }

    ITEM->_msgstrPlural[_pos.form].insert(_pos.offset,_str);

    _catalog->d->_posBuffer=_pos;
    _catalog->d->_posBuffer.offset+=_str.size();

}

void InsTextCmd::undo()
{
    ITEM->_msgstrPlural[_pos.form].remove(_pos.offset,_str.size());

    _catalog->d->_posBuffer=_pos;

    if ((!_pos.offset)&&(ITEM->_msgstrPlural[_pos.form].isEmpty()))
    {
        // insert index in the right place in the list
        QList<uint>::Iterator it = _catalog->d->_untransIndex.begin();
        while(it != _catalog->d->_untransIndex.end() && _pos.entry > (int)(*it))
            ++it;
        _catalog->d->_untransIndex.insert(it,_pos.entry);
        _catalog->emitsignalNumberOfUntranslatedChanged();
    }
}


DelTextCmd::DelTextCmd(Catalog *catalog,const DocPosition &pos,const QString &str)
    : QUndoCommand(i18nc("Undo command","Deletion"))
    , _catalog(catalog)
    , _str(str)
    , _pos(pos)
{
}

bool DelTextCmd::mergeWith(const QUndoCommand *other)
{
    if (
        (other->id() != id())
        || (static_cast<const DelTextCmd*>(other)->_pos.entry!=_pos.entry)
        || (static_cast<const DelTextCmd*>(other)->_pos.form!=_pos.form)
        )
        return false;

    //Delete
    if (static_cast<const DelTextCmd*>(other)->_pos.offset==_pos.offset)
    {
        _str += static_cast<const DelTextCmd*>(other)->_str;
        return true;
    }

    //BackSpace
    if (static_cast<const DelTextCmd*>(other)->_pos.offset==_pos.offset-static_cast<const DelTextCmd*>(other)->_str.size())
    {
        _str.prepend(static_cast<const DelTextCmd*>(other)->_str);
        _pos.offset=static_cast<const DelTextCmd*>(other)->_pos.offset;
        return true;
    }

    return false;
}
void DelTextCmd::redo()
{
    ITEM->_msgstrPlural[_pos.form].remove(_pos.offset,_str.size());

    _catalog->d->_posBuffer=_pos;
    _catalog->d->_posBuffer.offset+=_str.size();

    if ((!_pos.offset)&&(ITEM->_msgstrPlural.at(_pos.form).isEmpty()))
    {
        // insert index in the right place in the list
        QList<uint>::Iterator it = _catalog->d->_untransIndex.begin();
        while(it != _catalog->d->_untransIndex.end() && _pos.entry > (int)(*it))
            ++it;
        _catalog->d->_untransIndex.insert(it,_pos.entry);
        _catalog->emitsignalNumberOfUntranslatedChanged();
    }
}
void DelTextCmd::undo()
{
    if ((!_pos.offset)&&(ITEM->_msgstrPlural.at(_pos.form).isEmpty()))
    {
        _catalog->d->_untransIndex.removeAll(_pos.entry);
        _catalog->emitsignalNumberOfUntranslatedChanged();
    }


    ITEM->_msgstrPlural[_pos.form].insert(_pos.offset,_str);

    _catalog->d->_posBuffer=_pos;
}

ToggleFuzzyCmd::ToggleFuzzyCmd(Catalog *catalog,uint index,bool flag)
    : QUndoCommand(i18nc("Undo command","Fuzzy toggling"))
    , _catalog(catalog)
    , _index(index)
    , _flag(flag)
{
}

void ToggleFuzzyCmd::redo()
{
    if (_flag)
        setFuzzy();
    else
        unsetFuzzy();
}

void ToggleFuzzyCmd::undo()
{
    if (_flag)
        unsetFuzzy();
    else
        setFuzzy();
}

void ToggleFuzzyCmd::setFuzzy()
{
    DocPosition _pos;
    _pos.entry=_index;
    _pos.part=UndefPart;
    _catalog->d->_posBuffer=_pos;

    // insert index in the right place in the list
    QList<uint>::Iterator it = _catalog->d->_fuzzyIndex.begin();
    while(it != _catalog->d->_fuzzyIndex.end() && _index > short(*it))
        ++it;
    _catalog->d->_fuzzyIndex.insert(it,_index);
    _catalog->emitsignalNumberOfFuzziesChanged();

//     kWarning() << "BEFORE " << ITEM->_comment << endl;
    if (ITEM->_comment.isEmpty())
    {
        ITEM->_comment="#, fuzzy";
        return;
    }

    int p=ITEM->_comment.indexOf("#,");
    if(p!=-1)
    {
        ITEM->_comment.replace(p,2,"#, fuzzy,");
//             kWarning() << " 3AFETR " << ITEM->_comment << endl;
        return;
    }

    QRegExp a("\\#\\:[^\n]*\n");
    p=a.indexIn(ITEM->_comment);
    if (p!=-1)
    {
        ITEM->_comment.insert(p+a.matchedLength(),"#, fuzzy\n");
//             kWarning() << "1 AFETR " << ITEM->_comment << endl;
        return;
    }

    {
        if( !(ITEM->_comment.endsWith('\n')) )
            ITEM->_comment+='\n';
        ITEM->_comment+="#, fuzzy";
    }
//     kWarning() << "2 AFETR " << ITEM->_comment << endl;
}

void ToggleFuzzyCmd::unsetFuzzy()
{
    DocPosition _pos;
    _pos.entry=_index;
    _pos.part=UndefPart;
    _catalog->d->_posBuffer=_pos;

    _catalog->d->_fuzzyIndex.removeAll(_index);
    _catalog->emitsignalNumberOfFuzziesChanged();

//     kWarning() << "BEFORE " << ITEM->_comment << endl;
    ITEM->_comment.remove( QRegExp(",\\s*fuzzy"));

    // remove empty comment lines
    ITEM->_comment.remove( QRegExp("\n#\\s*$") );
    ITEM->_comment.remove( QRegExp("^#\\s*$") );
    ITEM->_comment.remove( QRegExp("#\\s*\n") );
    ITEM->_comment.remove( QRegExp("^#\\s*\n") );

//     kWarning() << "AFETR " << ITEM->_comment << endl;

}

