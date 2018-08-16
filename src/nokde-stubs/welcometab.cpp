/* ****************************************************************************
  This file is part of Lokalize

  Copyright (C) 2014 by Nick Shaforostoff <shafff@ukr.net>

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

#include "welcometab.h"
#include "prefs_lokalize.h"
#include "languagelistmodel.h"
#include "tmscanapi.h"
#include "project.h"
#include "catalog.h"
#include <klocalizedstring.h>

#include <QSortFilterProxyModel>
#include <QMimeData>
#include <QDragEnterEvent>

WelcomeTab::WelcomeTab(QWidget *parent)
    : LokalizeSubwindowBase2(parent)
{
#ifndef Q_OS_DARWIN
    menuBar()->hide();
#endif
    setWindowTitle("Lokalize"/*i18nc("@title:window","Lokalize")*/);//setCaption(i18nc("@title:window","Project"),false);
    setAcceptDrops(true);
    setCentralWidget(new QWidget(this));

    setupUi(centralWidget());

    QStringList i;
    i << i18n("Translator") << i18n("Reviewer") << i18n("Approver");
    roleCombo->addItems(i);
    roleCombo->setCurrentIndex(Project::instance()->local()->role());
    connect(roleCombo, SIGNAL(currentIndexChanged(int)), Project::instance()->local(), SLOT(setRole(int)));

    sourceLangCombo->setModel(LanguageListModel::instance()->sortModel());
    targetLangCombo->setModel(LanguageListModel::instance()->sortModel());
    sourceLangCombo->setCurrentIndex(LanguageListModel::instance()->sortModelRowForLangCode(Project::instance()->sourceLangCode()));
    targetLangCombo->setCurrentIndex(LanguageListModel::instance()->sortModelRowForLangCode(Project::instance()->targetLangCode()));
    LangCodeSaver* s = new LangCodeSaver(this);
    LangCodeSaver* t = new LangCodeSaver(this);
    connect(sourceLangCombo, SIGNAL(currentIndexChanged(int)), s, SLOT(setLangCode(int)));
    connect(targetLangCombo, SIGNAL(currentIndexChanged(int)), t, SLOT(setLangCode(int)));
    connect(s, SIGNAL(langCodeSelected(QString)), Project::instance(), SLOT(setSourceLangCode(QString)));
    connect(t, SIGNAL(langCodeSelected(QString)), Project::instance(), SLOT(setTargetLangCode(QString)));
    connect(t, SIGNAL(langCodeSelected(QString)), Settings::self(), SLOT(setDefaultLangCode(QString)));

    authorNameEdit->setText(Settings::self()->authorName());
    connect(authorNameEdit, SIGNAL(textChanged(QString)), Settings::self(), SLOT(setAuthorName(QString)));

    glossaryPathEdit->setText(Project::instance()->glossaryPath());
}

WelcomeTab::~WelcomeTab()
{
}

void LangCodeSaver::setLangCode(int index)
{
    emit langCodeSelected(LanguageListModel::instance()->langCodeForSortModelRow(index));
}

void WelcomeTab::dragEnterEvent(QDragEnterEvent* event)
{
    if (dragIsAcceptable(event->mimeData()->urls()))
        event->acceptProposedAction();
}

void WelcomeTab::dropEvent(QDropEvent* event)
{
    foreach (const QUrl& url, event->mimeData()->urls()) {
        const QString& filePath = url.toLocalFile();
        if (Catalog::extIsSupported(filePath)
            && Project::instance()->fileOpen(filePath)) {
            event->acceptProposedAction();
        }
    }
}

