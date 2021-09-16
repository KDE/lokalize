/* ****************************************************************************
  This file is part of Lokalize

  SPDX-FileCopyrightText: 2009-2014 Nick Shaforostoff <shafff@ukr.net>
                2018-2019 by Simon Depiets <sdepiets@gmail.com>

  SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL

**************************************************************************** */

#include "languagelistmodel.h"

#include <klanguagename.h>
#include <klocalizedstring.h>

#include <QStringBuilder>
#include <QCoreApplication>
#include <QSortFilterProxyModel>
#include <QStandardPaths>
#include <QLocale>
#include <QIcon>
#include <QSet>



LanguageListModel* LanguageListModel::_instance = nullptr;
LanguageListModel* LanguageListModel::_emptyLangInstance = nullptr;
void LanguageListModel::cleanupLanguageListModel()
{
    delete LanguageListModel::_instance; LanguageListModel::_instance = nullptr;
    delete LanguageListModel::_emptyLangInstance; LanguageListModel::_emptyLangInstance = nullptr;
}

LanguageListModel* LanguageListModel::instance()
{
    if (_instance == nullptr) {
        _instance = new LanguageListModel();
        qAddPostRoutine(LanguageListModel::cleanupLanguageListModel);
    }
    return _instance;
}

LanguageListModel* LanguageListModel::emptyLangInstance()
{
    if (_emptyLangInstance == nullptr)
        _emptyLangInstance = new LanguageListModel(WithEmptyLang);
    return _emptyLangInstance;
}

LanguageListModel::LanguageListModel(ModelType type, QObject* parent)
    : QStringListModel(parent)
    , m_sortModel(new QSortFilterProxyModel(this))
{
    setStringList(KLanguageName::allLanguageCodes());

    if (type == WithEmptyLang) insertRows(rowCount(), 1);
#if 0 //KDE5PORT
    KIconLoader::global()->addExtraDesktopThemes();
#endif
    //qCWarning(LOKALIZE_LOG)<<KIconLoader::global()->hasContext(KIconLoader::International);
    //qCDebug(LOKALIZE_LOG)<<KIconLoader::global()->queryIconsByContext(KIconLoader::NoGroup,KIconLoader::International);
    m_sortModel->setSourceModel(this);
    m_sortModel->setSortLocaleAware(true);
    m_sortModel->sort(0);
}

QVariant LanguageListModel::data(const QModelIndex& index, int role) const
{
    if (role == Qt::DecorationRole) {
#if 0 //#
        static QMap<QString, QVariant> iconCache;

        QString langCode = stringList().at(index.row());
        if (!iconCache.contains(langCode)) {
            QString code = QLocale(langCode).name();
            QString path;
            if (code.contains('_')) code = QString::fromRawData(code.unicode() + 3, 2).toLower();
            if (code != "C") {
                static const QString flagPath("l10n/%1/flag.png");
                path = QStandardPaths::locate(QStandardPaths::GenericDataLocation, QStringLiteral("locale/") + flagPath.arg(code));
            }
            iconCache[langCode] = QIcon(path);
        }
        return iconCache.value(langCode);
#endif
    } else if (role == Qt::DisplayRole) {
        const QString& code = stringList().at(index.row());
        if (code.isEmpty()) return code;
        //qCDebug(LOKALIZE_LOG)<<"languageCodeToName"<<code;
        static QVector<QString> displayNames(stringList().size());
        if (displayNames.at(index.row()).length())
            return displayNames.at(index.row());
        return QVariant::fromValue<QString>(
                   displayNames[index.row()] = i18nc("%1 is a language name, e.g. Irish, %2 is language code, e.g. ga", "%1 (%2)", KLanguageName::nameForCode(code), code));
    }
    return QStringListModel::data(index, role);
}

QFlags< Qt::ItemFlag > LanguageListModel::flags(const QModelIndex& index) const
{
    return QStringListModel::flags(index);
}

int LanguageListModel::sortModelRowForLangCode(const QString& langCode)
{
    return m_sortModel->mapFromSource(index(stringList().indexOf(langCode))).row();
}

QString LanguageListModel::langCodeForSortModelRow(int row)
{
    return stringList().at(m_sortModel->mapToSource(m_sortModel->index(row, 0)).row());
}


#include "prefs.h"
#include "project.h"
#include <klocalizedstring.h>
#include <QHBoxLayout>
#include <QLabel>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QDialog>

QString getTargetLangCode(const QString& title, bool askUser)
{
    if (!askUser) {
        if (Project::instance()->targetLangCode().length())
            return Project::instance()->targetLangCode();
        return QLocale::system().name();
    }

    QDialog dlg(SettingsController::instance()->mainWindowPtr());
    dlg.setWindowTitle(title);
    QHBoxLayout* l = new QHBoxLayout(&dlg);
    l->addWidget(new QLabel(i18n("Target language:"), &dlg));
    QComboBox* lc = new QComboBox(&dlg);
    l->addWidget(lc);
    lc->setModel(LanguageListModel::instance()->sortModel());
    lc->setCurrentIndex(LanguageListModel::instance()->sortModelRowForLangCode(Project::instance()->targetLangCode()));
    QDialogButtonBox* btn = new QDialogButtonBox(QDialogButtonBox::Ok, &dlg);
    l->addWidget(btn);
    QObject::connect(btn, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
    QObject::connect(btn, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);

    dlg.show();
    dlg.activateWindow(); //if we're called from another app
    if (!dlg.exec())
        return Project::instance()->targetLangCode();

    return LanguageListModel::instance()->langCodeForSortModelRow(lc->currentIndex());
}



