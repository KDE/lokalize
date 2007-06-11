/*****************************************************************************
  This file is part of KAider
  This file contains parts of KBabel code

  Copyright (C) 1999-2000 by Matthias Kiefer <matthias.kiefer@gmx.de>
		2001-2004 by Stanislav Visnovsky <visnovsky@kde.org>
		2007	  by Nick Shaforostoff <shafff@ukr.net>

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





#ifndef DATAMODEL_H
#define DATAMODEL_H

#include <QVector>
#include <QLinkedList>
#include <QUndoStack>

#include <kurl.h>

#include "pos.h"
#include "catalogitem.h"
#include "catalogfileplugin.h"
#include "catalog_private.h"
#include "pluralformtypes_enum.h"

#define CHECK_IF_EMPTY(_return_type)\
if (  d->_entries.isEmpty() )\
        return _return_type;


/**
* This class represents a catalog, saved in a po-file.
* It has the ability to load from and save to a po-file.
* Also it defines all necessary functions to set and get the entries
*
* @short Singleton class, that represents a translation catalog(po-file)
* @author Nick Shaforostoff <shafff@ukr.net>
*/
class Catalog: public QUndoStack
{
    Q_OBJECT

public:
    Catalog();
    ~Catalog();

    //ConversionStatus populateFromPO(const QString& file);

    QString msgstr(uint index, const uint form=0, const bool noNewlines=false) const;
    QString msgid(uint index, const uint form=0, const bool noNewlines=false) const;

    QString comment(uint index) const;
    QString msgctxt(uint index) const;

    PluralFormType pluralFormType(uint index) const;
    int numberOfPluralForms() const {return d->_numberOfPluralForms;};
    int numberOfEntries() const {return d->_entries.size();};
    int numberOfFuzzies() const {return d->_fuzzyIndex.size();};
    int numberOfUntranslated() const {return d->_untransIndex.size();};
    bool isFuzzy(uint index) const;
    bool isUntranslated(uint index) const;
    int firstFuzzyIndex() const {return d->_fuzzyIndex.isEmpty()?numberOfEntries():d->_fuzzyIndex.first();};
    int lastFuzzyIndex() const {return d->_fuzzyIndex.isEmpty()?-1:d->_fuzzyIndex.last();};
    int nextFuzzyIndex(uint index) const {return findNextInList(d->_fuzzyIndex,index);};
    int prevFuzzyIndex(uint index) const {return findPrevInList(d->_fuzzyIndex,index);};
    int firstUntranslatedIndex() const {return d->_untransIndex.isEmpty()?numberOfEntries():d->_untransIndex.first();};
    int lastUntranslatedIndex() const {return d->_untransIndex.isEmpty()?-1:d->_untransIndex.last();};
    int nextUntranslatedIndex(uint index) const {return findNextInList(d->_untransIndex,index);};
    int prevUntranslatedIndex(uint index) const {return findPrevInList(d->_untransIndex,index);};

    void clear();

    void setCatalogExtraData(const QStringList& data){d->_catalogExtraData = data;};
    QStringList catalogExtraData() const {return d->_catalogExtraData;};

    void setFileCodec( QTextCodec* codec ){d->fileCodec = codec;}
    QTextCodec* fileCodec() const {return d->fileCodec;}

    void setPackageName( QString s ){d->_packageName = s;}
    //QString _packageName() const {return d->fileCodec;}


    int maxLineLength(){return d->_maxLineLength>70?d->_maxLineLength:-1;}

    void setErrorIndex(const QList<uint>& errors){d->_errorIndex=errors;};

    void setImportPluginID(const QString& id){d->_importID=id;};
    QString importPluginID() const {return d->_importID;}

    void setMimeTypes(const QString& mimeTypes){d->_mimeTypes=mimeTypes;};

    bool setHeader(CatalogItem header);
    CatalogItem header() const {return d->_header;};

    KUrl url() const {return d->_url;};
    ConversionStatus openUrl(const KUrl& url/*, const QString& package*/);
    
    void updateHeader(bool forSaving=true);

public/* slots*/:
    virtual DocPosition& undo();
    virtual DocPosition& redo();

private:
    int findPrevInList(const QList<uint>& list,uint index) const;
    int findNextInList(const QList<uint>& list,uint index) const;
    void emitsignalNumberOfFuzziesChanged(){emit signalNumberOfFuzziesChanged();};
    void emitsignalNumberOfUntranslatedChanged(){emit signalNumberOfUntranslatedChanged();};
    bool setNumberOfPluralFormsFromHeader();

private:
    CatalogPrivate *d;

    friend class CatalogImportPlugin;
    friend class CatalogExportPlugin;
    friend class InsTextCmd;
    friend class DelTextCmd;
    friend class ToggleFuzzyCmd;
//    class GettextExportPlugin

signals:
    //void signalGotoEntry(const DocPosition& pos,int);
    void signalNumberOfFuzziesChanged();
    void signalNumberOfUntranslatedChanged();
    
private:
    static Catalog* _instance;

public:
    static Catalog* instance();
};

#endif

