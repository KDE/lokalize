/* ****************************************************************************
  This file is part of Lokalize

  Copyright (C) 2008-2009 by Nick Shaforostoff <shafff@ukr.net>

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

#include "catalogstring.h"
#include <kdebug.h>
#include <klocale.h>


const char* InlineTag::getElementName(InlineElement type)
{
    static const char* inlineElementNames[(int)InlineElementCount]={
    "_unknown",
    "bpt",
    "ept",
    "ph",
    "it",
    //"_NEVERSHOULDBECHOSEN",
    "mrk",
    "g",
    "sub",
    "_NEVERSHOULDBECHOSEN",
    "x",
    "bx",
    "ex"
    };

    return inlineElementNames[(int)type];
}

InlineTag InlineTag::getPlaceholder() const
{
    InlineTag tagRange=*this;
    tagRange.start=-1;
    tagRange.end=-1;
    return tagRange;
}

InlineTag::InlineElement InlineTag::getElementType(const QByteArray& tag)
{
    int i=InlineTag::InlineElementCount;
    while(--i>0)
        if (getElementName(InlineElement(i))==tag)
            break;
    return InlineElement(i);
}


QString InlineTag::displayName() const
{
    static const char* inlineElementNames[(int)InlineElementCount]={
        "_unknown",
        I18N_NOOP2("XLIFF inline tag name", "Start of paired tag"),
        I18N_NOOP2("XLIFF inline tag name", "End of paired tag"),
        I18N_NOOP2("XLIFF inline tag name", "Stand-alone tag"),
        I18N_NOOP2("XLIFF inline tag name", "Isolated tag"),
        //"_NEVERSHOULDBECHOSEN",
        I18N_NOOP2("XLIFF inline tag name", "Marker"),
        I18N_NOOP2("XLIFF inline tag name", "Generic group placeholder"),
        I18N_NOOP2("XLIFF inline tag name", "Sub-flow"),
        "_NEVERSHOULDBECHOSEN",
        I18N_NOOP2("XLIFF inline tag name", "Generic placeholder"),
        I18N_NOOP2("XLIFF inline tag name", "Start of paired placeholder"),
        I18N_NOOP2("XLIFF inline tag name", "End of paired placeholder")
    };

    QString result=i18nc("XLIFF inline tag name", inlineElementNames[type]);

    if (type==mrk)
    {
        static const char* mrkTypes[]={
            "abbrev",
            "abbreviated-form",
            "abbreviation",
            "acronym",
            "appellation",
            "collocation",
            "common-name",
            "datetime",
            "equation",
            "expanded-form",
            "formula",
            "head-term",
            "initialism",
            "international-scientific-term",
            "internationalism",
            "logical-expression",
            "materials-management-unit",
            "name",
            "near-synonym",
            "part-number",
            "phrase",
            "phraseological-unit",
            "protected",
            "romanized-form",
            "seg",
            "set-phrase",
            "short-form",
            "sku",
            "standard-text",
            "symbol",
            "synonym",
            "synonymous-phrase",
            "term",
            "transcribed-form",
            "transliterated-form",
            "truncated-term",
            "variant"
        };

        static const char* mrkTypeNames[]={
            I18N_NOOP2("XLIFF mark type", "abbreviation"),
            I18N_NOOP2("XLIFF mark type", "abbreviated form: a term resulting from the omission of any part of the full term while designating the same concept"),
            I18N_NOOP2("XLIFF mark type", "abbreviation: an abbreviated form of a simple term resulting from the omission of some of its letters (e.g. 'adj.' for 'adjective')"),
            I18N_NOOP2("XLIFF mark type", "acronym: an abbreviated form of a term made up of letters from the full form of a multiword term strung together into a sequence pronounced only syllabically (e.g. 'radar' for 'radio detecting and ranging')"),
            I18N_NOOP2("XLIFF mark type", "appellation: a proper-name term, such as the name of an agency or other proper entity"),
            I18N_NOOP2("XLIFF mark type", "collocation: a recurrent word combination characterized by cohesion in that the components of the collocation must co-occur within an utterance or series of utterances, even though they do not necessarily have to maintain immediate proximity to one another"),
            I18N_NOOP2("XLIFF mark type", "common name: a synonym for an international scientific term that is used in general discourse in a given language"),
            I18N_NOOP2("XLIFF mark type", "date and/or time"),
            I18N_NOOP2("XLIFF mark type", "equation: an expression used to represent a concept based on a statement that two mathematical expressions are, for instance, equal as identified by the equal sign (=), or assigned to one another by a similar sign"),
            I18N_NOOP2("XLIFF mark type", "expanded form: The complete representation of a term for which there is an abbreviated form"),
            I18N_NOOP2("XLIFF mark type", "formula: figures, symbols or the like used to express a concept briefly, such as a mathematical or chemical formula"),
            I18N_NOOP2("XLIFF mark type", "head term: the concept designation that has been chosen to head a terminological record"),
            I18N_NOOP2("XLIFF mark type", "initialism: an abbreviated form of a term consisting of some of the initial letters of the words making up a multiword term or the term elements making up a compound term when these letters are pronounced individually (e.g. 'BSE' for 'bovine spongiform encephalopathy')"),
            I18N_NOOP2("XLIFF mark type", "international scientific term: a term that is part of an international scientific nomenclature as adopted by an appropriate scientific body"),
            I18N_NOOP2("XLIFF mark type", "internationalism: a term that has the same or nearly identical orthographic or phonemic form in many languages"),
            I18N_NOOP2("XLIFF mark type", "logical expression: an expression used to represent a concept based on mathematical or logical relations, such as statements of inequality, set relationships, Boolean operations, and the like"),
            I18N_NOOP2("XLIFF mark type", "materials management unit: a unit to track object"),
            I18N_NOOP2("XLIFF mark type", "name"),
            I18N_NOOP2("XLIFF mark type", "near synonym: a term that represents the same or a very similar concept as another term in the same language, but for which interchangeability is limited to some contexts and inapplicable in others"),
            I18N_NOOP2("XLIFF mark type", "part number: a unique alphanumeric designation assigned to an object in a manufacturing system"),
            I18N_NOOP2("XLIFF mark type", "phrase"),
            I18N_NOOP2("XLIFF mark type", "phraseological: a group of two or more words that form a unit, the meaning of which frequently cannot be deduced based on the combined sense of the words making up the phrase"),
            I18N_NOOP2("XLIFF mark type", "protected: the marked text should not be translated"),
            I18N_NOOP2("XLIFF mark type", "romanized form: a form of a term resulting from an operation whereby non-Latin writing systems are converted to the Latin alphabet"),
            I18N_NOOP2("XLIFF mark type", "segment: the marked text represents a segment"),
            I18N_NOOP2("XLIFF mark type", "set phrase: a fixed, lexicalized phrase"),
            I18N_NOOP2("XLIFF mark type", "short form: a variant of a multiword term that includes fewer words than the full form of the term (e.g. 'Group of Twenty-four' for 'Intergovernmental Group of Twenty-four on International Monetary Affairs')"),
            I18N_NOOP2("XLIFF mark type", "stock keeping unit: an inventory item identified by a unique alphanumeric designation assigned to an object in an inventory control system"),
            I18N_NOOP2("XLIFF mark type", "standard text: a fixed chunk of recurring text"),
            I18N_NOOP2("XLIFF mark type", "symbol: a designation of a concept by letters, numerals, pictograms or any combination thereof"),
            I18N_NOOP2("XLIFF mark type", "synonym: a term that represents the same or a very similar concept as the main entry term in a term entry"),
            I18N_NOOP2("XLIFF mark type", "synonymous phrase: phraseological unit in a language that expresses the same semantic content as another phrase in that same language"),
            I18N_NOOP2("XLIFF mark type", "term"),
            I18N_NOOP2("XLIFF mark type", "transcribed form: a form of a term resulting from an operation whereby the characters of one writing system are represented by characters from another writing system, taking into account the pronunciation of the characters converted"),
            I18N_NOOP2("XLIFF mark type", "transliterated form: a form of a term resulting from an operation whereby the characters of an alphabetic writing system are represented by characters from another alphabetic writing system"),
            I18N_NOOP2("XLIFF mark type", "truncated term: an abbreviated form of a term resulting from the omission of one or more term elements or syllables (e.g. 'flu' for 'influenza')"),
            I18N_NOOP2("XLIFF mark type", "variant: one of the alternate forms of a term")
        };
        int i=sizeof(mrkTypes)/sizeof(char*);
        while(--i>=0 && mrkTypes[i]!=id)
            ;
        if (i!=-1)
        {
            result=i18nc("XLIFF mark type", mrkTypeNames[i]);
            if (!result.isEmpty())
                result[0]=result.at(0).toUpper();
        }
    }
    
    if (!ctype.isEmpty())
        result+=" ("+ctype+")";

    return result;
}


QMap<QString,int> CatalogString::tagIdToIndex() const
{
    QMap<QString,int> result;
    int index=0;
    int count=tags.size();
    for (int i=0;i<count;++i)
    {
        if (!result.contains(tags.at(i).id))
            result.insert(tags.at(i).id, index++);
    }
    return result;
}

QByteArray CatalogString::tagsAsByteArray()const
{
    QByteArray result;
    if (tags.size())
    {
        QDataStream stream(&result,QIODevice::WriteOnly);
        stream<<tags;
    }
    return result;
}

CatalogString::CatalogString(QString str, QByteArray tagsByteArray)
    : string(str)
{
    if (tagsByteArray.size())
    {
        QDataStream stream(tagsByteArray);
        stream>>tags;
    }
}

static void adjustTags(QList<InlineTag>& tags, int position, int value)
{
    int i=tags.size();
    while(--i>=0)
    {
        InlineTag& t=tags[i];
        if (t.start>position)
            t.start+=value;
        if (t.end>=position) //cases when strict > is needed?
            t.end+=value;
    }
}

void CatalogString::remove(int position, int len)
{
    string.remove(position,len);
    adjustTags(tags,position,-len);
}

void CatalogString::insert(int position, const QString& str)
{
    string.insert(position, str);
    adjustTags(tags,position,str.size());
}


QDataStream &operator<<(QDataStream &out, const InlineTag &t)
{
    return out<<int(t.type)<<t.start<<t.end<<t.id;
}

QDataStream &operator>>(QDataStream &in, InlineTag &t)
{
    int type;
    in>>type>>t.start>>t.end>>t.id;
    t.type=InlineTag::InlineElement(type);
    return in;
}

QDataStream &operator<<(QDataStream &out, const CatalogString &myObj)
{
    return out<<myObj.string<<myObj.tags;
}
QDataStream &operator>>(QDataStream &in, CatalogString &myObj)
{
    return in>>myObj.string>>myObj.tags;
}



void adaptCatalogString(CatalogString& target, const CatalogString& ref)
{
    kWarning()<<"HERE"<<target.string;
    QHash<QString,int> id2tagIndex;
    QMultiMap<InlineTag::InlineElement,int> tagType2tagIndex;
    int i=ref.tags.size();
    while(--i>=0)
    {
        const InlineTag& t=ref.tags.at(i);
        id2tagIndex.insert(t.id,i);
        tagType2tagIndex.insert(t.type,i);
        kWarning()<<"inserting"<<t.id<<t.type<<i;
    }

    QList<InlineTag> oldTags=target.tags;
    target.tags.clear();
    //we actually walking from beginning to end:
    qSort(oldTags.begin(), oldTags.end(), qGreater<InlineTag>());
    i=oldTags.size();
    while(--i>=0)
    {
        const InlineTag& targetTag=oldTags.at(i);
        if (id2tagIndex.contains(targetTag.id))
        {
            kWarning()<<"matched"<<targetTag.id<<i;
            target.tags.append(targetTag);
            tagType2tagIndex.remove(targetTag.type, id2tagIndex.take(targetTag.id));
            oldTags.removeAt(i);
        }
    }
    kWarning()<<"HERE 0"<<target.string;

    //now all the tags left have to ID (exact) matches
    i=oldTags.size();
    while(--i>=0)
    {
        InlineTag targetTag=oldTags.at(i);
        if (tagType2tagIndex.contains(targetTag.type))
        {
            //try to match by position
            //we're _taking_ first so the next one becomes new 'first' for the next time.
            QList<InlineTag> possibleRefMatches;
            foreach(int i, tagType2tagIndex.values(targetTag.type))
                possibleRefMatches<<ref.tags.at(i);
            qSort(possibleRefMatches);
            kWarning()<<"setting id:"<<targetTag.id<<possibleRefMatches.first().id;
            targetTag.id=possibleRefMatches.first().id;

            target.tags.append(targetTag);
            kWarning()<<"id??:"<<targetTag.id<<target.tags.first().id;
            tagType2tagIndex.remove(targetTag.type, id2tagIndex.take(targetTag.id));
            oldTags.removeAt(i);
        }
    }
    kWarning()<<"HERE 1"<<target.string;
    //now walk through unmatched tags and properly remove them.
    foreach(const InlineTag& tag, oldTags)
    {
        if (tag.isPaired())
            target.remove(tag.end, 1);
        target.remove(tag.start, 1);
    }
    kWarning()<<"HERE 2"<<target.string;
}

