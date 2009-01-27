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


#ifndef GETTEXTSTORAGE_H
#define GETTEXTSTORAGE_H

#include <QVector>
#include "catalogitem.h"
#include "catalogstorage.h"

/**
 * Implementation of Gettext PO format support
 */
namespace GettextCatalog {
    
/**
 * @short Implementation of storage for Gettext PO
 * @author Nick Shaforostoff <shafff@ukr.net>
 */
class GettextStorage: public CatalogStorage
{
public:
    GettextStorage();
    ~GettextStorage();

    bool load(QIODevice* device/*, bool readonly=false*/);
    bool save(QIODevice* device);

    int size() const;
    void clear();
    bool isEmpty() const;

    //flat-model interface (ignores XLIFF grouping)
    QString source(const DocPosition& pos) const;
    QString target(const DocPosition& pos) const;
    CatalogString sourceWithTags(const DocPosition& pos) const{return source(pos);}
    CatalogString targetWithTags(const DocPosition& pos) const{return target(pos);}

    void targetDelete(const DocPosition& pos, int count);
    void targetInsert(const DocPosition& pos, const QString& arg);
    void setTarget(const DocPosition& pos, const QString& arg);//called for mergeCatalog

    QStringList sourceAllForms(const DocPosition& pos) const;
    QStringList targetAllForms(const DocPosition& pos) const;

    //DocPosition.form - number of <note>
    QString note(const DocPosition& pos) const;
    int noteCount(const DocPosition& pos) const;

    //DocPosition.form - number of <context>
    QString context(const DocPosition& pos) const;
    int contextCount(const DocPosition& pos) const;

    QStringList matchData(const DocPosition& pos) const;
    QString id(const DocPosition& pos) const;

    bool isPlural(const DocPosition& pos) const;

    bool isApproved(const DocPosition& pos) const;
    void setApproved(const DocPosition& pos, bool approved);

    bool isUntranslated(const DocPosition& pos) const;

    QString mimetype()const{return "text/x-gettext-translation";}

private:
    bool setHeader(const CatalogItem& newHeader);

private:
    QVector<CatalogItem> m_entries;
    QVector<CatalogItem> m_obsoleteEntries;
    CatalogItem m_header;

    short m_maxLineLength;
    short m_trailingNewLines;
    bool m_generatedFromDocbook;

    QStringList m_catalogExtraData;
    QByteArray m_catalogExtraDataCompressed;

    friend class CatalogImportPlugin;
    friend class GettextExportPlugin;
};

}

#endif
