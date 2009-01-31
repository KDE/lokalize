/*
Copyright 2008 Nick Shaforostoff <shaforostoff@kde.ru>

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
*/


#ifndef CATALOGSTORAGE_H
#define CATALOGSTORAGE_H

#include "pos.h"
#include "catalogstring.h"

#include <kurl.h>
#include <QStringList>

/**
 * Abstract interface for storage of translation file
 *
 * format-specific elements like \" for gettext PO should be eliminated
 *
 * @short Abstract interface for storage of translation file
 * @author Nick Shaforostoff <shafff@ukr.net>
*/
class CatalogStorage {
public:
    CatalogStorage();
    virtual ~CatalogStorage();

    virtual bool load(QIODevice* device)=0;
    virtual bool save(QIODevice* device)=0;

    virtual int size() const=0;
    int numberOfEntries()const{return size();}
    virtual void clear()=0;//TODO remove this
    virtual bool isEmpty() const=0;//TODO remove this

    int numberOfPluralForms() const{return m_numberOfPluralForms;}

    /**
     * flat-model interface (ignores XLIFF grouping)
     *
     * format-specific texts like \" for gettext PO should be eliminated
    **/
    virtual QString source(const DocPosition& pos) const=0;
    virtual QString target(const DocPosition& pos) const=0;
    virtual CatalogString sourceWithTags(DocPosition pos) const=0;
    virtual CatalogString targetWithTags(DocPosition pos) const=0;
    virtual CatalogString catalogString(const DocPosition& pos) const=0;

    /**
     * edit operations used by undo/redo  system and sync-mode
    **/
    virtual void targetDelete(const DocPosition& pos, int count)=0;
    virtual void targetInsert(const DocPosition& pos, const QString& arg)=0;
    virtual void setTarget(const DocPosition& pos, const QString& arg)=0;//called for mergeCatalog
    virtual void targetInsertTag(const DocPosition& pos, const TagRange& tag)=0;
    virtual TagRange targetDeleteTag(const DocPosition& pos)=0;

    /**
     * all plural forms
     *
     * pos.form doesn't matter
    **/
    virtual QStringList sourceAllForms(const DocPosition& pos) const=0;
    virtual QStringList targetAllForms(const DocPosition& pos) const=0;

    //DocPosition.form - number of <note>
    virtual QString note(const DocPosition& pos) const=0;
    virtual int noteCount(const DocPosition& pos) const=0;

    //DocPosition.form - number of <context>
    virtual QString context(const DocPosition& pos) const=0;
    virtual int contextCount(const DocPosition& pos) const=0;

    /**
     * user-invisible data for matching, e.g. during TM database lookup
     * it is comprised of several strings
     *
     * database stores <!--hashes of each of--> them and thus it is possible to
     * fuzzy-match 'matchData' later
     *
     * it is responsibility of CatalogStorage implementations to
     * separate/assemble the list properly according to the format specifics
     *
     * pos.form doesn't matter
    **/
    virtual QStringList matchData(const DocPosition& pos) const=0;

    /**
     * entry id unique for this file
     *
     * pos.form doesn't matter
    **/
    virtual QString id(const DocPosition& pos) const=0;

    virtual bool isPlural(const DocPosition& pos) const=0;

    virtual bool isUntranslated(const DocPosition& pos) const=0;

    virtual bool isApproved(const DocPosition& pos) const=0;
    virtual void setApproved(const DocPosition& pos, bool approved)=0;


    const KUrl& url() const {return m_url;}
    void setUrl(const KUrl& u){m_url=u;}//TODO

    virtual QString mimetype()const=0;

protected:
    KUrl m_url;
    QString m_langCode;
    int m_numberOfPluralForms;
};

inline CatalogStorage::CatalogStorage()
    : m_numberOfPluralForms(0)
{
}

inline CatalogStorage::~CatalogStorage()
{
}





#endif
