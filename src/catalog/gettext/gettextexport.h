/* ****************************************************************************
  This file is part of Lokalize
  This file contains parts of KBabel code

  Copyright (C) 2002 by Stanislav Visnovsky <visnovsky@kde.org>
  Copyright (C) 2006 by Nicolas GOUTTE <goutte@kde.org>

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
#ifndef GETTEXTEXPORTPLUGIN_H
#define GETTEXTEXPORTPLUGIN_H

#include <catalogfileplugin.h>

#include <QStringList>
#include <QTextStream>

namespace GettextCatalog {
class GettextStorage;


/**
 * @brief The class for exporting GNU gettext PO files.
 *
 * As an extra information, it stores the list of all obsolete entries.
 */

class GettextExportPlugin
{
public:
    GettextExportPlugin(short wrapWidth=-1, short trailingNewLines=1);
    ConversionStatus save(QIODevice* device,
                          const GettextStorage* catalog);

private:
    /**
     * Write a PO comment to @p stream and take care that each comment lines start with a # character
     */
    void writeComment( QTextStream& stream, const QString& comment ) const;

    /**
     * Write a PO keyword (msgctxt, msgid, msgstr, msgstr_plural, msgstr[0]) and the corresponding text.
     * This includes wrapping the text.
     */
    void writeKeyword( QTextStream& stream, const QString& keyword, QString text, bool containsHtml=true, bool startedWithEmptyLine=false ) const;

public:
    /**
     * @brief Width of the wrap
     *
     * This is the width of the wrap in characters (not bytes), including everything
     * (e.g. keyword, quote characters, spaces).
     *
     * - A value of 0 means no wrap
     * - A value of -1 means the traditional KBabel wrapping
     * - Other negative values are reserved for future extensions (by default: no wrap)
     * @note
     * - Gettext's default value is 78 characters
     * - Too small values might not be correctly supported.
     */
    short m_wrapWidth;
    short m_trailingNewLines;
};
}
#endif
