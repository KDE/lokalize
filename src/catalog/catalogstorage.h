/*
Copyright 2008-2009 Nick Shaforostoff <shaforostoff@kde.ru>

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
#include "note.h"
#include "state.h"
#include "phase.h"
#include "alttrans.h"
#include "catalogcapabilities.h"

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

    virtual int capabilities() const=0;

    virtual int load(QIODevice* device)=0;
    virtual bool save(QIODevice* device, bool belongsToProject=false)=0;

    virtual int size() const=0;
    int numberOfEntries()const{return size();}
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
    virtual void setTarget(const DocPosition& pos, const QString& arg)=0;//called for mergeCatalog TODO switch to CatalogString
    virtual void targetInsertTag(const DocPosition&, const InlineTag&){}
    virtual InlineTag targetDeleteTag(const DocPosition&){return InlineTag();}
    virtual Phase updatePhase(const Phase&){return Phase();}
    virtual QList<Phase> allPhases() const{return QList<Phase>();}
    virtual QMap<QString,Tool> allTools() const{return QMap<QString,Tool>();}

    /// all plural forms. pos.form doesn't matter
    virtual QStringList sourceAllForms(const DocPosition& pos, bool stripNewLines=false) const=0;
    virtual QStringList targetAllForms(const DocPosition& pos, bool stripNewLines=false) const=0;

    virtual QVector<AltTrans> altTrans(const DocPosition& pos) const=0;
    virtual QVector<Note> notes(const DocPosition& pos) const=0;
    virtual Note setNote(DocPosition pos, const Note& note)=0;
    virtual QStringList noteAuthors() const{return QStringList();}
    virtual QVector<Note> developerNotes(const DocPosition& pos) const=0;
    virtual QStringList sourceFiles(const DocPosition& pos) const=0;

    virtual QString setPhase(const DocPosition& pos, const QString& phase){Q_UNUSED(pos); Q_UNUSED(phase); return QString();}
    virtual QString phase(const DocPosition& pos) const {Q_UNUSED(pos); return QString();}
    virtual Phase phase(const QString& name) const{Q_UNUSED(name); return Phase();}
    virtual QVector<Note> phaseNotes(const QString& phase) const{Q_UNUSED(phase); return QVector<Note>();}
    virtual QVector<Note> setPhaseNotes(const QString& phase, QVector<Note> notes){Q_UNUSED(phase); Q_UNUSED(notes); return QVector<Note>();}

    //the result must be guaranteed to have at least 1 string
    virtual QStringList context(const DocPosition&) const=0;
    //DocPosition.form - number of <context>
    //virtual QString context(const DocPosition&) const=0;
    //virtual int contextCount(const DocPosition&) const=0;

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
    virtual QStringList matchData(const DocPosition&) const=0;

    /**
     * entry id unique for this file
     *
     * pos.form doesn't matter
    **/
    virtual QString id(const DocPosition&) const=0;

    virtual bool isPlural(const DocPosition&) const=0;

    virtual bool isEmpty(const DocPosition&) const=0;

    virtual bool isEquivTrans(const DocPosition&) const{return true;}
    virtual void setEquivTrans(const DocPosition&, bool equivTrans){Q_UNUSED(equivTrans);}

    virtual bool isApproved(const DocPosition&) const{return true;}
    virtual void setApproved(const DocPosition&, bool approved){Q_UNUSED(approved);}
    virtual TargetState state(const DocPosition&) const{return New;}
    virtual TargetState setState(const DocPosition&, TargetState){return New;}


    virtual int binUnitsCount() const {return 0;}
    virtual int unitById(const QString& id) const {Q_UNUSED(id); return 0;}

    const KUrl& url() const {return m_url;}
    void setUrl(const KUrl& u){m_url=u;}//TODO

    virtual QString mimetype() const=0;

    QString sourceLangCode() const{return m_sourceLangCode;}
    QString targetLangCode() const{return m_targetLangCode;}

protected:
    KUrl m_url;
    QString m_sourceLangCode;
    QString m_targetLangCode;

    int m_numberOfPluralForms;
};

inline CatalogStorage::CatalogStorage()
    : m_sourceLangCode("en_US")
    , m_numberOfPluralForms(0)
{
}

inline CatalogStorage::~CatalogStorage()
{
}





#endif
