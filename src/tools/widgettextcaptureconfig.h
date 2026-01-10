/*
  This file is part of Lokalize

  SPDX-FileCopyrightText: 2012      Nick Shaforostoff <shafff@ukr.net>
  SPDX-FileCopyrightText: 2018-2019 Simon Depiets <sdepiets@gmail.com>

  SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#ifndef WIDGETTEXTCAPTURECONFIG_H
#define WIDGETTEXTCAPTURECONFIG_H

#include <QDialog>

class Ui_WidgetTextCapture;

class WidgetTextCaptureConfig : public QDialog
{
    Q_OBJECT
public:
    explicit WidgetTextCaptureConfig(QWidget *parent = nullptr);
    ~WidgetTextCaptureConfig() override;
public Q_SLOTS:
    void writeConfig();

private:
    Ui_WidgetTextCapture *const ui;
};

#endif // WIDGETTEXTCAPTURECONFIG_H
