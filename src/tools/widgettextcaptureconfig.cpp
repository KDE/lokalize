/* ****************************************************************************
  This file is part of Lokalize

  Copyright (C) 2012 by Nick Shaforostoff <shafff@ukr.net>

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

#include "widgettextcaptureconfig.h"
#include "ui_widgettextcaptureconfig.h"

WidgetTextCaptureConfig::WidgetTextCaptureConfig(QWidget* parent)
 : KDialog(parent)
 , ui(new Ui_WidgetTextCapture)
{
    setAttribute(Qt::WA_DeleteOnClose, true);
    ui->setupUi(mainWidget());
    setCaption(i18nc("@title","Widget Text Capture"));
    setButtons(KDialog::Ok | KDialog::Cancel);

    KConfigGroup cg(KGlobal::config(), "Development");
    bool copyWidgetText = cg.readEntry("CopyWidgetText", false);
    QString copyWidgetTextCommand = cg.readEntry("CopyWidgetTextCommand", QString());
    ui->none->setChecked(!copyWidgetText);
    ui->clipboard->setChecked(copyWidgetText && copyWidgetTextCommand.isEmpty());
    ui->search->setChecked(copyWidgetText && !copyWidgetTextCommand.isEmpty());

    connect(this,SIGNAL(accepted()),this,SLOT(writeConfig()));
}

WidgetTextCaptureConfig::~WidgetTextCaptureConfig()
{
    delete ui;
}

void WidgetTextCaptureConfig::writeConfig()
{
    KConfig konfig(QString::fromLatin1("kdeglobals"), KConfig::NoGlobals);
    KConfigGroup cg=konfig.group("Development");
    cg.writeEntry("CopyWidgetText", !ui->none->isChecked());
    if (ui->clipboard->isChecked())
        cg.writeEntry("CopyWidgetTextCommand", QString());
    else if (ui->search->isChecked())
        cg.writeEntry("CopyWidgetTextCommand", "/bin/sh `kde4-config --path data --locate lokalize/scripts/find-gui-text.sh` \"%1\" \"%2\"");

    konfig.sync();
}
