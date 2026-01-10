/*
  This file is part of Lokalize

  SPDX-FileCopyrightText: 2022 Andreas Cord-Landwehr <cordlandwehr@kde.org>

  SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef TEST_GETTEXTHEADERPARSER_H
#define TEST_GETTEXTHEADERPARSER_H

#include <QObject>

class TestGetTextHeaderParser : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void updateLastTranslatorAddTranslator();
    void updateLastTranslatorReplaceTranslator();
    void updateGenericCopyrightYear();
    void updateAuthorsWithNameEmail();
    void updateAuthorsWithNameEmailYear();
    void updateAuthorsWithNameEmailYearYear();
    void updateAuthorsWithCopyrightNameEmail();
    void updateAuthorsWithCopyrightYearNameEmail();
    void updateAuthorsWithCopyrightYearYearNameEmail();
    void updateAuthorsInitialAddition();
    void updateAuthorsAddNewCopyrightOwner();
    void updateAuthorsTestModifyExistingCopyrightOwner();
    void updateAuthorsCopyrightText();

private:
    static const QString sCurrentYear;
};
#endif
