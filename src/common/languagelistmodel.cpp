/* ****************************************************************************
  This file is part of Lokalize

  Copyright (C) 2009-2014 by Nick Shaforostoff <shafff@ukr.net>
                2018-2019 by Simon Depiets <sdepiets@gmail.com>

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

#include "languagelistmodel.h"

#include <kconfig.h>
#include <kconfiggroup.h>

#include <QStringBuilder>
#include <QCoreApplication>
#include <QSortFilterProxyModel>
#include <QStandardPaths>
#include <QLocale>
#include <QIcon>
#include <QSet>



LanguageListModel* LanguageListModel::_instance = 0;
LanguageListModel* LanguageListModel::_emptyLangInstance = 0;
void LanguageListModel::cleanupLanguageListModel()
{
    delete LanguageListModel::_instance; LanguageListModel::_instance = 0;
    delete LanguageListModel::_emptyLangInstance; LanguageListModel::_emptyLangInstance = 0;
}

LanguageListModel* LanguageListModel::instance()
{
    if (_instance == 0) {
        _instance = new LanguageListModel();
        qAddPostRoutine(LanguageListModel::cleanupLanguageListModel);
    }
    return _instance;
}

LanguageListModel* LanguageListModel::emptyLangInstance()
{
    if (_emptyLangInstance == 0)
        _emptyLangInstance = new LanguageListModel(WithEmptyLang);
    return _emptyLangInstance;
}

LanguageListModel::LanguageListModel(ModelType type, QObject* parent)
    : QStringListModel(parent)
    , m_sortModel(new QSortFilterProxyModel(this))
    , m_systemLangList(new KConfig(QLatin1String("locale/kf5_all_languages"), KConfig::NoGlobals, QStandardPaths::GenericDataLocation))
{
    setStringList(m_systemLangList->groupList());

    if (type == WithEmptyLang) insertRows(rowCount(), 1);
#if 0 //KDE5PORT
    KIconLoader::global()->addExtraDesktopThemes();
#endif
    //qCWarning(LOKALIZE_LOG)<<KIconLoader::global()->hasContext(KIconLoader::International);
    //qCDebug(LOKALIZE_LOG)<<KIconLoader::global()->queryIconsByContext(KIconLoader::NoGroup,KIconLoader::International);
    m_sortModel->setSourceModel(this);
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
                   displayNames[index.row()] = KConfigGroup(m_systemLangList, code).readEntry("Name") + QStringLiteral(" (") + code + ')');
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



