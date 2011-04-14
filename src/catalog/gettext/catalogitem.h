/* ****************************************************************************
  This file is part of Lokalize
  This file is based on the one from KBabel

  Copyright (C) 1999-2000 by Matthias Kiefer <matthias.kiefer@gmx.de>
  Copyright (C) 2002-2003 by Stanislav Visnovsky <visnovsky@kde.org>
  Copyright (C) 2006 by Nicolas GOUTTE <goutte@kde.org>
  Copyright (C) 2007-2011 by Nick Shaforostoff <shafff@ukr.net>

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
#ifndef CATALOGITEM_H
#define CATALOGITEM_H

#include <QStringList>

namespace GettextCatalog {

class CatalogItemPrivate;

/**
 * This class represents an entry in a catalog.
 * It contains the comment, the Msgid and the Msgstr.
 * It defines some functions to query the state of the entry
 * (fuzzy, untranslated, cformat).
 *
 * @short Represents an entry in a Gettext catalog
 * @author Matthias Kiefer <matthias.kiefer@gmx.de>
 * @author Nick Shaforostoff <shafff@ukr.net>
 */
class CatalogItem
{

public:
    explicit CatalogItem();
    CatalogItem(const CatalogItem&);
    ~CatalogItem();

    bool isFuzzy() const;      //", fuzzy" in comment
    bool isCformat() const;    //", c-format" or possible-c-format in comment (from the debug parameter of xgettext)
    bool isNoCformat() const;  //", no-c-format" in comment
    bool isQtformat() const;   //", qt-format" in comment
    bool isNoQtformat() const; //", no-qt-format" in comment
    bool isUntranslated() const;
    bool isUntranslated(uint form) const;


    bool isPlural() const;
    void setPlural(bool plural=true);

    void setSyntaxError(bool);

    /** returns the number of lines, the entry will need in a file */
    int totalLines() const;
 
    /** cleares the item */
    void clear();

    QString comment() const;
    const QString& msgctxt(const bool noNewlines = false) const;
    const QString& msgid(const int form=0) const;
    const QString& msgstr(const int form=0) const;
    const QVector<QString>& msgstrPlural() const;
    enum Part {Source, Target};
    QStringList allPluralForms(CatalogItem::Part, bool stripNewLines=false) const;
    bool prependEmptyForMsgid(const int form=0) const;
    bool prependEmptyForMsgstr(const int form=0) const;

    QStringList msgstrAsList() const;
    void setComment(const QString& com);
    void setMsgctxt(const QString& msg);
    void setMsgid(const QString& msg, const int form=0);
    void setMsgid(const QStringList& msg);
    void setMsgid(const QStringList& msg, bool prependEmptyLine);
    void setMsgid(const QVector<QString>& msg);
    void setMsgstr(const QString& msg, const int form=0);
    void setMsgstr(const QStringList& msg);
    void setMsgstr(const QStringList& msg, bool prependEmptyLine);
    void setMsgstr(const QVector<QString>& msg);

    void setValid(bool);
    bool isValid() const;
#if 0
	/**
	 * @return the list of all errors of this item 
	 */
	QStringList errors() const;
	
	QString nextError() const;
	void clearErrors();
	void removeError(const QString& error);
	void appendError(const QString& error);

	/**
	 * makes some sanity checks and set status accordingly
	 * @return the new status of this item
	 * @see CatalogItem::Error
	 * @param accelMarker a char, that marks the keyboard accelerators
	 * @param contextInfo a regular expression, that determines what is 
	 * the context information
	 * @param singularPlural a regular expression, that determines what is 
	 * string with singular and plural form
     * @param neededLines how many lines a string with singular-plural form
     * must have
	 */
	int checkErrors(QChar accelMarker, const QRegExp& contextInfo
            , const QRegExp& singularPlural, const int neededLines);
	
#endif
    void operator=(const CatalogItem& rhs);

private:
    CatalogItemPrivate* const d;

    friend class GettextStorage;
    void setFuzzy();
    void unsetFuzzy();

};

}

#endif // CATALOGITEM_H
