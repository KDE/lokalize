/*
    SPDX-License-Identifier: GPL-2.0-or-later
    SPDX-FileCopyrightText: 2022 Andreas Cord-Landwehr <cordlandwehr@kde.org>
*/

#ifndef TEST_GETTEXTHEADERPARSER_H
#define TEST_GETTEXTHEADERPARSER_H

#include <QObject>

class TestGetTextHeaderParser : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void updateLastTranslator();
    void updateGenericCopyrightYear();
    void updateAuthors();

private:
    static const QString sCurrentYear;
};
#endif
