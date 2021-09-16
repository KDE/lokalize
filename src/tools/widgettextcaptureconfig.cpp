/*
  This file is part of Lokalize

  SPDX-FileCopyrightText: 2012 Nick Shaforostoff <shafff@ukr.net>
                2018-2019 by Simon Depiets <sdepiets@gmail.com>

  SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL

*/

#include "widgettextcaptureconfig.h"
#include "ui_widgettextcaptureconfig.h"
#include <klocalizedstring.h>
#include <kconfiggroup.h>
#include <ksharedconfig.h>
#include <kconfig.h>

WidgetTextCaptureConfig::WidgetTextCaptureConfig(QWidget* parent)
    : QDialog(parent)
    , ui(new Ui_WidgetTextCapture)
{
    setAttribute(Qt::WA_DeleteOnClose, true);
    ui->setupUi(this);
    setWindowTitle(i18nc("@title", "Widget Text Capture"));

    KConfigGroup cg(KSharedConfig::openConfig(), "Development");
    bool copyWidgetText = cg.readEntry("CopyWidgetText", false);
    QString copyWidgetTextCommand = cg.readEntry("CopyWidgetTextCommand", QString());
    ui->none->setChecked(!copyWidgetText);
    ui->clipboard->setChecked(copyWidgetText && copyWidgetTextCommand.isEmpty());
    ui->search->setChecked(copyWidgetText && !copyWidgetTextCommand.isEmpty());

    connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &WidgetTextCaptureConfig::accept);
    connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &WidgetTextCaptureConfig::reject);
    connect(this, &WidgetTextCaptureConfig::accepted, this, &WidgetTextCaptureConfig::writeConfig);
}

WidgetTextCaptureConfig::~WidgetTextCaptureConfig()
{
    delete ui;
}

void WidgetTextCaptureConfig::writeConfig()
{
    KConfig konfig(QLatin1String("kdeglobals"), KConfig::NoGlobals);
    KConfigGroup cg = konfig.group("Development");
    cg.writeEntry("CopyWidgetText", !ui->none->isChecked());
    if (ui->clipboard->isChecked())
        cg.writeEntry("CopyWidgetTextCommand", QString());
    else if (ui->search->isChecked())
        cg.writeEntry("CopyWidgetTextCommand", "/bin/sh /usr/share/lokalize/scripts/find-gui-text.sh \"%1\" \"%2\"");

    konfig.sync();
}
