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

#include <kurl.h>
#include <QStringList>

/**
    Interface for storage
	@author Nick Shaforostoff <shafff@ukr.net>
*/
class CatalogStorage {
public:
    CatalogStorage();
    virtual ~CatalogStorage();

    virtual bool load(const KUrl& url)=0;
    virtual bool save(const KUrl& url=KUrl())=0;

    virtual int size() const=0;
    int numberOfEntries()const{return size();}
    virtual void clear()=0;
    virtual bool isEmpty() const=0;

    int numberOfPluralForms() const{return m_numberOfPluralForms;}

    //flat-model interface (ignores XLIFF grouping)
    virtual const QString& source(const DocPosition& pos) const=0;
    virtual const QString& target(const DocPosition& pos) const=0;

    virtual void targetDelete(const DocPosition& pos, int count)=0;
    virtual void targetInsert(const DocPosition& pos, const QString& arg)=0;
    virtual void setTarget(const DocPosition& pos, const QString& arg)=0;

    virtual QStringList sourceAllForms(const DocPosition& pos) const=0;
    virtual QStringList targetAllForms(const DocPosition& pos) const=0;

    //DocPosition.form - number of <note>
    virtual const QString& note(const DocPosition& pos) const=0;
    virtual int noteCount(const DocPosition& pos) const=0;

    //DocPosition.form - number of <context>
    virtual const QString& context(const DocPosition& pos) const=0;
    virtual int contextCount(const DocPosition& pos) const=0;

    virtual QStringList matchData(const DocPosition& pos) const=0;
    virtual QString id(const DocPosition& pos) const=0;

    virtual bool isPlural(const DocPosition& pos) const=0;

    virtual bool isApproved(const DocPosition& pos) const=0;
    virtual void setApproved(const DocPosition& pos, bool)=0;

    virtual bool isUntranslated(const DocPosition& pos) const=0;



    const KUrl& url() const {return m_url;}
    void setUrl(const KUrl& u){m_url=u;}//TODO

    const QString& langCode() const {return m_langCode;}
    void setLangCode(const QString& lc){m_langCode=lc;}//TODO

    const QString& language() const {return m_language;}
    void setLanguage(const QString& l){m_language=l;}//TODO

protected:
    KUrl m_url;
    QString m_langCode;
    QString m_language;
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
