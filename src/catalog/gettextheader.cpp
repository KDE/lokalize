/*
  This file is part of Lokalize

  SPDX-FileCopyrightText: 2008-2014 Nick Shaforostoff <shafff@ukr.net>
  SPDX-FileCopyrightText: 2018-2019 Simon Depiets <sdepiets@gmail.com>

  SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#include "gettextheader.h"

#include "lokalize_debug.h"

#include "gettextheaderparser.h"
#include "project.h"

#include "version.h"
#include "prefs_lokalize.h"
#include "prefs.h"

#include <QInputDialog>
#include <QProcess>
#include <QThread>
#include <QString>
#include <QStringBuilder>
#include <QMap>
#include <QTextCodec>
#include <QDateTime>
#include <QTimeZone>

#include <klocalizedstring.h>

/**
 * this data was obtained by running GNUPluralForms()
 * on all languages KDE knows of
**/

struct langPInfo {
    const char *lang;
    const char *plural;
};

static const langPInfo langsWithPInfo[] = {
    { "ar", "nplurals=6; plural=n==0 ? 0 : n==1 ? 1 : n==2 ? 2 : n%100>=3 && n%100<=10 ? 3 : n%100>=11 && n%100<=99 ? 4 : 5;" },
    { "be@latin", "nplurals=3; plural=(n%10==1 && n%100!=11 ? 0 : n%10>=2 && n%10<=4 && (n%100<10 || n%100>=20) ? 1 : 2);" },
    { "be", "nplurals=3; plural=(n%10==1 && n%100!=11 ? 0 : n%10>=2 && n%10<=4 && (n%100<10 || n%100>=20) ? 1 : 2);" },
    { "bg", "nplurals=2; plural=(n != 1);" },
    { "br", "nplurals=2; plural=(n > 1);" },
    { "bs", "nplurals=3; plural=n%10==1 && n%100!=11 ? 0 : n%10>=2 && n%10<=4 && (n%100<10 || n%100>=20) ? 1 : 2;" },
    { "csb", "nplurals=3; plural=(n==1 ? 0 : n%10>=2 && n%10<=4 && (n%100<10 || n%100>=20) ? 1 : 2)" },
    { "cs", "nplurals=3; plural=(n==1) ? 0 : (n>=2 && n<=4) ? 1 : 2;" },
    { "da", "nplurals=2; plural=(n != 1);" },
    { "de", "nplurals=2; plural=(n != 1);" },
    { "el", "nplurals=2; plural=(n != 1);" },
    { "en", "nplurals=2; plural=(n != 1);" },
    { "en_GB", "nplurals=2; plural=(n != 1);" },
    { "en_US", "nplurals=2; plural=(n != 1);" },
    { "eo", "nplurals=2; plural=(n != 1);" },
    { "es", "nplurals=2; plural=(n != 1);" },
    { "et", "nplurals=2; plural=(n != 1);" },
    { "fa", "nplurals=2; plural=(n > 1);" },
    { "fi", "nplurals=2; plural=(n != 1);" },
    { "fo", "nplurals=2; plural=(n != 1);" },
    { "fr", "nplurals=2; plural=(n > 1);" },
    { "ga", "nplurals=5; plural=n==1 ? 0 : n==2 ? 1 : n<7 ? 2 : n < 11 ? 3 : 4" },
    { "gd", "nplurals=4; plural=(n==1 || n==11) ? 0 : (n==2 || n==12) ? 1 : (n > 2 && n < 20) ? 2 : 3;" },
    { "gu", "nplurals=2; plural=(n!=1);" },
    { "he", "nplurals=2; plural=(n != 1);" },
    { "hi", "nplurals=2; plural=(n!=1);" },
    { "hne", "nplurals=2; plural=(n!=1);" },
    { "hr", "nplurals=3; plural=(n%10==1 && n%100!=11 ? 0 : n%10>=2 && n%10<=4 && (n%100<10 || n%100>=20) ? 1 : 2);" },
    { "hsb", "nplurals=4; plural=n%100==1 ? 0 : n%100==2 ? 1 : n%100==3 || n%100==4 ? 2 : 3;" },
    { "hu", "nplurals=2; plural=(n != 1);" },
    { "hy", "nplurals=4; plural=n==1 ? 3 : n%10==1 && n%100!=11 ? 0 : n%10>=2 && n%10<=4 && (n%100<10 || n%100>=20) ? 1 : 2;" },
    { "id", "nplurals=2; plural=(n != 1);" },
    { "it", "nplurals=2; plural=(n != 1);" },
    { "ja", "nplurals=1; plural=0;" },
    { "ka", "nplurals=1; plural=0;" },
    { "kk", "nplurals=1; plural=0;" },
    { "km", "nplurals=1; plural=0;" },
    { "ko", "nplurals=1; plural=0;" },
    { "lt", "nplurals=3; plural=(n%10==1 && n%100!=11 ? 0 : n%10>=2 && (n%100<10 || n%100>=20) ? 1 : 2);" },
    { "lv", "nplurals=3; plural=(n%10==1 && n%100!=11 ? 0 : n != 0 ? 1 : 2);" },
    { "mai", "nplurals=2; plural=(n!=1);" },
    { "mk", "nplurals=3; plural=n%10==1 ? 0 : n%10==2 ? 1 : 2;" },
    { "mr", "nplurals=2; plural=(n!=1);" },
    { "ms", "nplurals=2; plural=1;" },
    { "nb", "nplurals=2; plural=(n != 1);" },
    { "nl", "nplurals=2; plural=(n != 1);" },
    { "nn", "nplurals=2; plural=(n != 1);" },
    { "oc", "nplurals=2; plural=(n > 1);" },
    { "or", "nplurals=2; plural=(n!=1);" },
    { "pl", "nplurals=3; plural=(n==1 ? 0 : n%10>=2 && n%10<=4 && (n%100<10 || n%100>=20) ? 1 : 2);" },
    { "pt", "nplurals=2; plural=(n != 1);" },
    { "pt_BR", "nplurals=2; plural=(n > 1);" },
    { "ro", "nplurals=3; plural=n==1 ? 0 : (n==0 || (n%100 > 0 && n%100 < 20)) ? 1 : 2;" },
    { "ru", "nplurals=3; plural=(n%10==1 && n%100!=11 ? 0 : n%10>=2 && n%10<=4 && (n%100<10 || n%100>=20) ? 1 : 2);" },
    { "sk", "nplurals=3; plural=(n==1) ? 0 : (n>=2 && n<=4) ? 1 : 2;" },
    { "sl", "nplurals=4; plural=(n%100==1 ? 1 : n%100==2 ? 2 : n%100==3 || n%100==4 ? 3 : 0);" },
    { "sr", "nplurals=3; plural=(n%10==1 && n%100!=11 ? 0 : n%10>=2 && n%10<=4 && (n%100<10 || n%100>=20) ? 1 : 2);" },
    { "sr@latin", "nplurals=3; plural=(n%10==1 && n%100!=11 ? 0 : n%10>=2 && n%10<=4 && (n%100<10 || n%100>=20) ? 1 : 2);" },
    { "sv", "nplurals=2; plural=(n != 1);" },
    { "te", "nplurals=2; plural=(n != 1);" },
    { "th", "nplurals=1; plural=0;" },
    { "tr", "nplurals=2; plural=(n > 1);" },
    { "ug", "nplurals=1; plural=0;" },
    { "uk", "nplurals=3; plural=(n%10==1 && n%100!=11 ? 0 : n%10>=2 && n%10<=4 && (n%100<10 || n%100>=20) ? 1 : 2);" },
    { "uz", "nplurals=1; plural=0;" },
    { "uz@cyrillic", "nplurals=1; plural=0;" },
    { "vi", "nplurals=1; plural=0;" },
    { "zh_CN", "nplurals=1; plural=0;" },
    { "zh_HK", "nplurals=1; plural=0;" },
    { "zh_TW", "nplurals=1; plural=0;" }

};

static const size_t langsWithPInfoCount = sizeof(langsWithPInfo) / sizeof(langsWithPInfo[0]);


int numberOfPluralFormsFromHeader(const QString& header)
{
    QRegExp rxplural(QStringLiteral("Plural-Forms:\\s*nplurals=(.);"));
    if (rxplural.indexIn(header) == -1)
        return 0;
    bool ok{};
    int result = rxplural.cap(1).toShort(&ok);
    return ok ? result : 0;

}

int numberOfPluralFormsForLangCode(const QString& langCode)
{
    QString expr = GNUPluralForms(langCode);

    QRegExp rxplural(QStringLiteral("nplurals=(.);"));
    if (rxplural.indexIn(expr) == -1)
        return 0;
    bool ok;
    int result = rxplural.cap(1).toShort(&ok);
    return ok ? result : 0;

}

QString GNUPluralForms(const QString& lang)
{
    QByteArray l(lang.toUtf8());
    int i = langsWithPInfoCount;
    while (--i >= 0 && l != langsWithPInfo[i].lang)
        ;
    if (Q_LIKELY(i >= 0))
        return QString::fromLatin1(langsWithPInfo[i].plural);

    i = langsWithPInfoCount;
    while (--i >= 0 && !l.startsWith(langsWithPInfo[i].lang))
        ;
    if (Q_LIKELY(i >= 0))
        return QString::fromLatin1(langsWithPInfo[i].plural);


    //BEGIN alternative
    // NOTE does this work under M$ OS?
    qCDebug(LOKALIZE_LOG) << "gonna call msginit";
    QString def = QStringLiteral("nplurals=2; plural=n != 1;");

    QStringList arguments;
    arguments << QLatin1String("-l") << lang
              << QLatin1String("-i") << QLatin1String("-")
              << QLatin1String("-o") << QLatin1String("-")
              << QLatin1String("--no-translator")
              << QLatin1String("--no-wrap");

    const QString msginitFullPath = QStandardPaths::findExecutable(QLatin1String("msginit"));
    if (msginitFullPath.isEmpty()) {
        return def;
    }

    QProcess msginit;
    msginit.start(msginitFullPath, arguments);

    msginit.waitForStarted(5000);
    if (Q_UNLIKELY(msginit.state() != QProcess::Running)) {
        //qCWarning(LOKALIZE_LOG)<<"msginit error";
        return def;
    }

    msginit.write(
        "# SOME DESCRIPTIVE TITLE.\n"
        "# Copyright (C) YEAR Free Software Foundation, Inc.\n"
        "# FIRST AUTHOR <EMAIL@ADDRESS>, YEAR.\n"
        "#\n"
        "#, fuzzy\n"
        "msgid \"\"\n"
        "msgstr \"\"\n"
        "\"Project-Id-Version: PACKAGE VERSION\\n\"\n"
        "\"POT-Creation-Date: 2002-06-25 03:23+0200\\n\"\n"
        "\"PO-Revision-Date: YEAR-MO-DA HO:MI+ZONE\\n\"\n"
        "\"Last-Translator: FULL NAME <EMAIL@ADDRESS>\\n\"\n"
        "\"Language-Team: LANGUAGE <LL@li.org>\\n\"\n"
        "\"Language: LL\\n\"\n"
        "\"MIME-Version: 1.0\\n\"\n"
        "\"Content-Type: text/plain; charset=UTF-8\\n\"\n"
        "\"Content-Transfer-Encoding: ENCODING\\n\"\n"
//                   "\"Plural-Forms: nplurals=INTEGER; plural=EXPRESSION;\\n\"\n"
    );
    msginit.closeWriteChannel();

    if (Q_UNLIKELY(!msginit.waitForFinished(5000))) {
        qCWarning(LOKALIZE_LOG) << "msginit error";
        return def;
    }


    QByteArray result = msginit.readAll();
    int pos = result.indexOf("Plural-Forms: ");
    if (Q_UNLIKELY(pos == -1)) {
        //qCWarning(LOKALIZE_LOG)<<"msginit error"<<result;
        return def;
    }
    pos += 14;

    int end = result.indexOf('"', pos);
    if (Q_UNLIKELY(pos == -1)) {
        //qCWarning(LOKALIZE_LOG)<<"msginit error"<<result;
        return def;
    }

    return QString::fromLatin1(result.mid(pos, end - pos - 2));
    //END alternative
}

QString formatGettextDate(const QDateTime &dt)
{
    QLocale cLocale(QLocale::C);
    QString dateTimeString = cLocale.toString(dt, QStringLiteral("yyyy-MM-dd HH:mm"));
    const int offset_seconds = dt.offsetFromUtc();
    const int offset_hours = abs(offset_seconds) / 3600;
    const int offset_minutes = abs(offset_seconds % 3600) / 60;
    QString zoneOffsetString = (offset_seconds >= 0 ? QLatin1Char('+') : QLatin1Char('-')) + (offset_hours < 10 ? QStringLiteral("0") : QString()) + QString::number(offset_hours) + (offset_minutes < 10 ? QStringLiteral("0") : QString()) + QString::number(offset_minutes);

    return dateTimeString + zoneOffsetString;
}

void updateHeader(QString& header,
                  QString& comment,
                  QString& langCode,
                  int& numberOfPluralForms,
                  const QString& CatalogProjectId,
                  bool generatedFromDocbook,
                  bool belongsToProject,
                  bool forSaving,
                  QTextCodec* codec)
{
    askAuthorInfoIfEmpty();

    QStringList headerList(header.split(QLatin1Char('\n'), Qt::SkipEmptyParts));
    QStringList commentList(comment.split(QLatin1Char('\n'), Qt::SkipEmptyParts));

//BEGIN header itself
    QStringList::Iterator it, ait;
    QString temp;
    const QString BACKSLASH_N = QStringLiteral("\\n");

    // Unwrap header since the following code
    // assumes one header item per headerList element
    it = headerList.begin();
    while (it != headerList.end()) {
        if (!(*it).endsWith(BACKSLASH_N)) {
            const QString line = *it;
            it = headerList.erase(it);
            if (it != headerList.end()) {
                *it = line + *it;
            } else {
                // Something bad happened, put a warning on the command line
                qCWarning(LOKALIZE_LOG) << "Bad .po header, last header line was" << line;
            }
        } else {
            ++it;
        }
    }

    GetTextHeaderParser::updateLastTranslator(headerList, Settings::authorName(), Settings::authorEmail());

    bool found = false;
    temp = QStringLiteral("PO-Revision-Date: ") + formatGettextDate(QDateTime::currentDateTime()) + BACKSLASH_N;
    QRegExp poRevDate(QStringLiteral("^ *PO-Revision-Date:.*"));
    for (it = headerList.begin(), found = false; it != headerList.end() && !found; ++it) {
        found = it->contains(poRevDate);
        if (found && forSaving) *it = temp;
    }
    if (Q_UNLIKELY(!found))
        headerList.append(temp);

    temp = QStringLiteral("Project-Id-Version: ") + CatalogProjectId + BACKSLASH_N;
    //temp.replace( "@PACKAGE@", packageName());
    QRegExp projectIdVer(QStringLiteral("^ *Project-Id-Version:.*"));
    for (it = headerList.begin(), found = false; it != headerList.end() && !found; ++it) {
        found = it->contains(projectIdVer);
        if (found && it->contains(QLatin1String("PACKAGE VERSION")))
            *it = temp;
    }
    if (Q_UNLIKELY(!found))
        headerList.append(temp);


    langCode = Project::instance()->isLoaded() ?
               Project::instance()->langCode() :
               Settings::defaultLangCode();
    QString language; //initialized with preexisting value or later
    QString mailingList; //initialized with preexisting value or later

    static QMap<QString, QLocale::Language> langEnums;
    if (!langEnums.size())
        for (int l = QLocale::Abkhazian; l <= QLocale::Akoose; ++l)
            langEnums[QLocale::languageToString((QLocale::Language)l)] = (QLocale::Language)l;

    static QRegExp langTeamRegExp(QStringLiteral("^ *Language-Team:.*"));
    for (it = headerList.begin(), found = false; it != headerList.end() && !found; ++it) {
        found = it->contains(langTeamRegExp);
        if (found) {
            //really parse header
            QRegExp re(QStringLiteral("^ *Language-Team: *(.*) *<([^>]*)>"));
            if (re.indexIn(*it) != -1) {
                if (langEnums.contains(re.cap(1).trimmed())) {
                    language = re.cap(1).trimmed();
                    mailingList = re.cap(2).trimmed();
                    QList<QLocale> locales = QLocale::matchingLocales(langEnums.value(language), QLocale::AnyScript, QLocale::AnyCountry);
                    if (locales.size()) langCode = locales.first().name().left(2);
                }
            }

            ait = it;
        }
    }

    if (language.isEmpty()) {
        language = QLocale::languageToString(QLocale(langCode).language());
        if (language.isEmpty())
            language = langCode;
    }

    if (mailingList.isEmpty() || belongsToProject) {
        if (Project::instance()->isLoaded())
            mailingList = Project::instance()->mailingList();
        else //if (mailingList.isEmpty())
            mailingList = Settings::defaultMailingList();
    }

    Project::LangSource projLangSource = Project::instance()->languageSource();
    QString projLT = Project::instance()->projLangTeam();
    if (projLangSource == Project::LangSource::Project) {
        temp = QStringLiteral("Language-Team: ") + projLT + QStringLiteral("\\n");
    } else if ((projLangSource == Project::LangSource::Application) && (Settings::overrideLangTeam())) {
        temp = QStringLiteral("Language-Team: ") + Settings::userLangTeam() + QStringLiteral("\\n");
    } else {
        temp = QStringLiteral("Language-Team: ") + language + QStringLiteral(" <") + mailingList + QStringLiteral(">\\n");
    }
    if (Q_LIKELY(found))
        (*ait) = temp;
    else
        headerList.append(temp);

    static QRegExp langCodeRegExp(QStringLiteral("^ *Language: *([^ \\\\]*)"));
    temp = QStringLiteral("Language: ") + langCode + BACKSLASH_N;
    for (it = headerList.begin(), found = false; it != headerList.end() && !found; ++it) {
        found = (langCodeRegExp.indexIn(*it) != -1);
        if (found && langCodeRegExp.cap(1).isEmpty())
            *it = temp;
        //if (found) qCWarning(LOKALIZE_LOG)<<"got explicit lang code:"<<langCodeRegExp.cap(1);
    }
    if (Q_UNLIKELY(!found))
        headerList.append(temp);

    temp = QStringLiteral("Content-Type: text/plain; charset=") + QString::fromLatin1(codec->name()) + BACKSLASH_N;
    QRegExp ctRe(QStringLiteral("^ *Content-Type:.*"));
    for (it = headerList.begin(), found = false; it != headerList.end() && !found; ++it) {
        found = it->contains(ctRe);
        if (found) *it = temp;
    }
    if (Q_UNLIKELY(!found))
        headerList.append(temp);


    temp = QStringLiteral("Content-Transfer-Encoding: 8bit\\n");
    QRegExp cteRe(QStringLiteral("^ *Content-Transfer-Encoding:.*"));
    for (it = headerList.begin(), found = false; it != headerList.end() && !found; ++it)
        found = it->contains(cteRe);
    if (!found)
        headerList.append(temp);

    // ensure MIME-Version header
    temp = QStringLiteral("MIME-Version: 1.0\\n");
    QRegExp mvRe(QStringLiteral("^ *MIME-Version:"));
    for (it = headerList.begin(), found = false; it != headerList.end() && !found; ++it) {
        found = it->contains(mvRe);
        if (found) *it = temp;
    }
    if (Q_UNLIKELY(!found))
        headerList.append(temp);


    //qCDebug(LOKALIZE_LOG)<<"testing for GNUPluralForms";
    // update plural form header
    QRegExp pfRe(QStringLiteral("^ *Plural-Forms:"));
    for (it = headerList.begin(), found = false; it != headerList.end() && !found; ++it)
        found = it->contains(pfRe);
    if (found) {
        --it;

        //qCDebug(LOKALIZE_LOG)<<"GNUPluralForms found";
        int num = numberOfPluralFormsFromHeader(header);
        if (!num) {
            if (generatedFromDocbook)
                num = 1;
            else {
                qCDebug(LOKALIZE_LOG) << "No plural form info in header, using project-defined one" << langCode;
                QString t = GNUPluralForms(langCode);
                //qCWarning(LOKALIZE_LOG)<<"generated: " << t;
                if (!t.isEmpty()) {
                    static QRegExp pf(QStringLiteral("^ *Plural-Forms:\\s*nplurals.*\\\\n"));
                    pf.setMinimal(true);
                    temp = QStringLiteral("Plural-Forms: %1\\n").arg(t);
                    it->replace(pf, temp);
                    num = numberOfPluralFormsFromHeader(temp);
                } else {
                    qCWarning(LOKALIZE_LOG) << "no... smth went wrong :(\ncheck your gettext install";
                    num = 2;
                }
            }
        }
        numberOfPluralForms = num;

    } else if (!generatedFromDocbook) {
        //qCDebug(LOKALIZE_LOG)<<"generating GNUPluralForms"<<langCode;
        QString t = GNUPluralForms(langCode);
        //qCDebug(LOKALIZE_LOG)<<"here it is:";
        if (!t.isEmpty()) {
            const QString pluralFormLine = QStringLiteral("Plural-Forms: %1\\n").arg(t);
            headerList.append(pluralFormLine);
            numberOfPluralForms = numberOfPluralFormsFromHeader(pluralFormLine);
        }
    }

    temp = QStringLiteral("X-Generator: Lokalize %1\\n").arg(QStringLiteral(LOKALIZE_VERSION));
    QRegExp xgRe(QStringLiteral("^ *X-Generator:.*"));
    for (it = headerList.begin(), found = false; it != headerList.end() && !found; ++it) {
        found = it->contains(xgRe);
        if (found) *it = temp;
    }
    if (Q_UNLIKELY(!found))
        headerList.append(temp);

    header = headerList.join(QStringLiteral("\n"));
//END header itself

//BEGIN comment = description, copyrights
    GetTextHeaderParser::updateGeneralCopyrightYear(commentList);
    // qCDebug(LOKALIZE_LOG) << "HEADER COMMENT: " << commentList;

    GetTextHeaderParser::updateAuthors(commentList, Settings::authorName(), Settings::authorEmail());
    comment = commentList.join(QStringLiteral("\n"));

//END comment = description, copyrights
}



QString fullUserName();// defined in <platform>helpers.cpp

bool askAuthorInfoIfEmpty()
{
    if (QThread::currentThread() == qApp->thread()) {

        if (Settings::authorName().isEmpty()) {
            bool ok{};
            QString contact = QInputDialog::getText(
                                  SettingsController::instance()->mainWindowPtr(),
                                  i18nc("@window:title", "Author name missing"), i18n("Your name:"),
                                  QLineEdit::Normal, fullUserName(), &ok);

            Settings::self()->authorNameItem()->setValue(ok ? contact : fullUserName());
            Settings::self()->save();
        }
        if (Settings::authorEmail().isEmpty()) {
            bool ok;
            QString email = QInputDialog::getText(
                                SettingsController::instance()->mainWindowPtr(),
                                i18nc("@window:title", "Author email missing"), i18n("Your email:"),
                                QLineEdit::Normal, QString(), &ok);

            if (ok) {
                Settings::self()->authorEmailItem()->setValue(email);
                Settings::self()->save();
            }
        }
    }
    return !Settings::authorName().isEmpty() && !Settings::authorEmail().isEmpty();
}
