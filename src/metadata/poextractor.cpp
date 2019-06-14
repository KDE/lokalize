/*
    Gettext translation file analyzer

    Copyright (C) 2007 Montel Laurent <montel@kde.org>
    Copyright (C) 2009 Jos van den Oever <jos@vandenoever.info>
    Copyright (C) 2014 Nick Shaforostoff <shaforostoff@gmail.com>
                2018-2019 by Simon Depiets <sdepiets@gmail.com>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include "poextractor.h"

#include <fstream>

#include <QFile>

POExtractor::POExtractor()
    : state(WHITESPACE)
    , messages(0)
    , untranslated(0)
    , fuzzy(0)
    , isFuzzy(false)
    , isTranslated(false)
{
}

void POExtractor::endMessage()
{
    messages++;
    if (isTranslated) fuzzy += isFuzzy;
    untranslated += (!isTranslated);

    isFuzzy = false;
    isTranslated = false;
    state = WHITESPACE;
}

void POExtractor::handleComment(const char* data, uint32_t length)
{
    state = COMMENT;
    if (length >= 8 && strncmp(data, "#, fuzzy", 8) == 0) { // could be better
        isFuzzy = true;
    }
}

void POExtractor::handleLine(const char* data, uint32_t length)
{
    if (state == ERROR) return;
    if (state == WHITESPACE) {
        if (length == 0) return;
        if (data[0] != '#') {
            state = COMMENT; //this allows PO files w/o comments
        } else {
            handleComment(data, length);
            return;
        }
    }
    if (state == COMMENT) {
        if (length == 0) {
            state = WHITESPACE;
        } else if (data[0] == '#') {
            handleComment(data, length);
        } else if (length > 7 && strncmp("msgctxt", data, 7) == 0) {
            state = MSGCTXT;
        } else if (length > 7 && strncmp("msgid \"", data, 7) == 0) {
            state = MSGID;
        } else {
            state = ERROR;
        }
        return;
    } else if (length > 1 && data[0] == '"' && data[length - 1] == '"'
               && (state == MSGCTXT || state == MSGID || state == MSGSTR
                   || state == MSGID_PLURAL)) {
        // continued text field
        isTranslated = state == MSGSTR && length > 2;
    } else if (state == MSGCTXT
               && length > 7 && strncmp("msgid \"", data, 7) == 0) {
        state = MSGID;
    } else if (state == MSGID
               && length > 14 && strncmp("msgid_plural \"", data, 14) == 0) {
        state = MSGID_PLURAL;
    } else if ((state == MSGID || state == MSGID_PLURAL || state == MSGSTR)
               && length > 8 && strncmp("msgstr", data, 6) == 0) {
        state = MSGSTR;
        isTranslated = strncmp(data + length - 3, " \"\"", 3) != 0;
    } else if (state == MSGSTR) {
        if (length == 0) {
            endMessage();
        } else if (data[0] == '#' || data[0] == 'm') { //allow PO without empty line between entries
            endMessage();
            state = COMMENT;
            handleLine(data, length);
        } else {
            state = ERROR;
        }
    } else {
        state = ERROR;
    }
#if 0
    if (messages > 1 || state != MSGSTR) return;

    // handle special values in the first messsage
    // assumption is that value takes up only one line
    if (strncmp("\"POT-Creation-Date: ", data, 20) == 0) {
        result->add(Property::TranslationTemplateDate, QByteArray(data + 20, length - 21));
    } else if (strncmp("\"PO-Revision-Date: ", data, 19) == 0) {
        result->add(Property::TranslationLastUpDate, QByteArray(data + 19, length - 20));
    } else if (strncmp("\"Last-Translator: ", data, 18) == 0) {
        result->add(Property::TranslationLastAuthor, QByteArray(data + 18, length - 19));
    }
#endif
}

FileMetaData POExtractor::extract(const QString& filePath)
{
    std::ifstream fstream(QFile::encodeName(filePath));
    if (!fstream.is_open()) {
        return {};
    }

    state = WHITESPACE;
    messages = 0;
    untranslated = 0;
    fuzzy = 0;
    isFuzzy = false;
    isTranslated = false;

    std::string line;
    int lines = 0;
    FileMetaData m;
    while (std::getline(fstream, line)) {
        //TODO add a parsed text of translation units
        //QByteArray arr = QByteArray::fromRawData(line.c_str(), line.size());
        //result->append(QString::fromUtf8(arr));

        handleLine(line.c_str(), line.size());
        lines++;

        if (messages <= 1 && state == MSGSTR) {
            // handle special values in the first messsage
            // assumption is that value takes up only one line
            if (strncmp("\"POT-Creation-Date: ", line.c_str(), 20) == 0) {
                m.sourceDate = QByteArray(line.c_str() + 20, line.size() - 21 - 2);
            } else if (strncmp("\"PO-Revision-Date: ", line.c_str(), 19) == 0) {
                m.translationDate = QByteArray(line.c_str() + 19, line.size() - 20 - 2);
            } else if (strncmp("\"Last-Translator: ", line.c_str(), 18) == 0) {
                m.lastTranslator = QString::fromUtf8(QByteArray::fromRawData(line.c_str() + 18, line.size() - 19 - 2));
            }
            fuzzy = 0;
        }
    }
    handleLine("", 0); //for files with non-empty last line
    messages--;//cause header does not count

    /*
        result->add(Property::TranslationUnitsTotal, messages);
        result->add(Property::TranslationUnitsWithTranslation, messages-untranslated);
        result->add(Property::TranslationUnitsWithDraftTranslation, fuzzy);
        result->add(Property::LineCount, lines);
    */

    //TODO WordCount
    m.fuzzy      = fuzzy;
    m.translated = messages - untranslated - fuzzy;
    m.untranslated = untranslated;
    m.filePath = filePath;

    //File is invalid
    if (messages < 0 || fuzzy < 0 || untranslated < 0) {
        m.invalid_file = true;
        m.translated = 0;
        m.untranslated = 0;
        m.fuzzy = 0;
    }

    //TODO
    m.translated_approver = m.translated_reviewer = m.translated;
    m.fuzzy_approver = m.fuzzy_reviewer = m.fuzzy;

    return m;
}
