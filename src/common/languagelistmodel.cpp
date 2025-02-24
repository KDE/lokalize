/*
  This file is part of Lokalize

  SPDX-FileCopyrightText: 2009-2014 Nick Shaforostoff <shafff@ukr.net>
  SPDX-FileCopyrightText: 2018-2019 Simon Depiets <sdepiets@gmail.com>

  SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#include "languagelistmodel.h"

#include <klanguagename.h>
#include <klocalizedstring.h>

#include <QCoreApplication>
#include <QIcon>
#include <QLocale>
#include <QSet>
#include <QSortFilterProxyModel>
#include <QStandardPaths>
#include <QStringBuilder>

LanguageListModel *LanguageListModel::_instance = nullptr;
LanguageListModel *LanguageListModel::_emptyLangInstance = nullptr;
void LanguageListModel::cleanupLanguageListModel()
{
    delete LanguageListModel::_instance;
    LanguageListModel::_instance = nullptr;
    delete LanguageListModel::_emptyLangInstance;
    LanguageListModel::_emptyLangInstance = nullptr;
}

LanguageListModel *LanguageListModel::instance()
{
    if (_instance == nullptr) {
        _instance = new LanguageListModel();
        qAddPostRoutine(LanguageListModel::cleanupLanguageListModel);
    }
    return _instance;
}

LanguageListModel *LanguageListModel::emptyLangInstance()
{
    if (_emptyLangInstance == nullptr)
        _emptyLangInstance = new LanguageListModel(WithEmptyLang);
    return _emptyLangInstance;
}

LanguageListModel::LanguageListModel(ModelType type, QObject *parent)
    : QStringListModel(parent)
    , m_sortModel(new QSortFilterProxyModel(this))
{
    setStringList(KLanguageName::allLanguageCodes());

    if (type == WithEmptyLang)
        insertRows(rowCount(), 1);
    m_sortModel->setSourceModel(this);
    m_sortModel->setSortLocaleAware(true);
    m_sortModel->sort(0);
}

QVariant LanguageListModel::data(const QModelIndex &index, int role) const
{
    if (role == Qt::DecorationRole) {
    } else if (role == Qt::DisplayRole) {
        QString code = stringList().at(index.row());
        if (code.isEmpty())
            return code;
        static QVector<QString> displayNames(stringList().size());
        if (displayNames.at(index.row()).length())
            return displayNames.at(index.row());
        return QVariant::fromValue<QString>(
            displayNames[index.row()] =
                i18nc("%1 is a language name, e.g. Irish, %2 is language code, e.g. ga", "%1 (%2)", KLanguageName::nameForCode(code), code));
    }
    return QStringListModel::data(index, role);
}

QFlags<Qt::ItemFlag> LanguageListModel::flags(const QModelIndex &index) const
{
    return QStringListModel::flags(index);
}

int LanguageListModel::sortModelRowForLangCode(const QString &langCode)
{
    return m_sortModel->mapFromSource(index(stringList().indexOf(langCode))).row();
}

QString LanguageListModel::langCodeForSortModelRow(int row)
{
    return stringList().at(m_sortModel->mapToSource(m_sortModel->index(row, 0)).row());
}

#include "prefs.h"
#include "project.h"
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QHBoxLayout>
#include <QLabel>
#include <klocalizedstring.h>

QString getTargetLangCode(const QString &title, bool askUser)
{
    if (!askUser) {
        if (Project::instance()->targetLangCode().length())
            return Project::instance()->targetLangCode();
        return QLocale::system().name();
    }

    QDialog dlg(SettingsController::instance()->mainWindowPtr());
    dlg.setWindowTitle(title);
    QHBoxLayout *l = new QHBoxLayout(&dlg);
    l->addWidget(new QLabel(i18n("Target language:"), &dlg));
    QComboBox *lc = new QComboBox(&dlg);
    l->addWidget(lc);
    lc->setModel(LanguageListModel::instance()->sortModel());
    lc->setCurrentIndex(LanguageListModel::instance()->sortModelRowForLangCode(Project::instance()->targetLangCode()));
    QDialogButtonBox *btn = new QDialogButtonBox(QDialogButtonBox::Ok, &dlg);
    l->addWidget(btn);
    QObject::connect(btn, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
    QObject::connect(btn, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);

    dlg.show();
    dlg.activateWindow(); // if we're called from another app
    if (!dlg.exec())
        return Project::instance()->targetLangCode();

    return LanguageListModel::instance()->langCodeForSortModelRow(lc->currentIndex());
}
