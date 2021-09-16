/* ****************************************************************************
  This file is part of Lokalize
  This file contains parts of KBabel code

  SPDX-FileCopyrightText: 2002 Stanislav Visnovsky <visnovsky@kde.org>
  SPDX-FileCopyrightText: 2006 Nicolas GOUTTE <goutte@kde.org>

  SPDX-License-Identifier: GPL-2.0-or-later

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
#include <QTextCodec>
#include <QTextStream>

namespace GettextCatalog
{
class GettextStorage;


/**
 * @brief The class for exporting GNU gettext PO files.
 *
 * As an extra information, it stores the list of all obsolete entries.
 */

class GettextExportPlugin
{
public:
    explicit GettextExportPlugin(short wrapWidth = -1, short trailingNewLines = 1);
    ConversionStatus save(QIODevice* device,
                          const GettextStorage* catalog,
                          QTextCodec* codec);

private:
    /**
     * Write a PO comment to @p stream and take care that each comment lines start with a # character
     */
    void writeComment(QTextStream& stream, const QString& comment) const;

    /**
     * Write a PO keyword (msgctxt, msgid, msgstr, msgstr_plural, msgstr[0]) and the corresponding text.
     * This includes wrapping the text.
     */
    void writeKeyword(QTextStream& stream, const QString& keyword, QString text, bool containsHtml = true, bool startedWithEmptyLine = false) const;

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
