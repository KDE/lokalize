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

#ifndef GLOSSARY_H
#define GLOSSARY_H

#include <QStringList>
#include <QMultiHash>

/**
 * struct that contains types data we work with.
 * this data can also be added to the TBX file
 *
 * the entry represents term, not word(s),
 * so there can be only one subjectField.
 */
struct TermEntry
{
    QStringList english;
    QStringList target;
    QString definition;
    int subjectField; //index in global Glossary's subjectFields list
    QString id;       //used to identify entry on edit action
    //TODO <descrip type="context"></descrip>

    TermEntry(const QStringList& _english,
              const QStringList& _target,
              const QString& _definition,
              int _subjectField,
              const QString& _id=QString()
             )
    : english(_english)
    , target(_target)
    , definition(_definition)
    , subjectField(_subjectField)
    , id(_id)
    {}

    TermEntry()
    : subjectField(-1)
    {}

    void clear()
    {
        english.clear();
        target.clear();
        definition.clear();
        subjectField=-1;
    }
};



/**
 * internal representation of glossary.
 * we store only data we need (i.e. only subset of TBX format)
 */
struct Glossary
{
    QMultiHash<QString,int> wordHash;
    QList<TermEntry> termList;
    QStringList subjectFields;

    QString path;

    void load(const QString&);
    void add(const TermEntry&);
    void change(const TermEntry&);


    void clear()
    {
        wordHash.clear();
        termList.clear();
        subjectFields.clear();
        path.clear();
    }
};


#endif
