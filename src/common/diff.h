/*
  This file is part of Lokalize

  SPDX-FileCopyrightText: 2007      Nick Shaforostoff <shafff@ukr.net>
  SPDX-FileCopyrightText: 2018-2019 Simon Depiets <sdepiets@gmail.com>
  SPDX-FileCopyrightText: 2024      Finley Watson <fin-w@tutanota.com>

  SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

/**
 * wordDiff algorithm adoption and further refinement by Nick Shaforostoff
 * (based on Markus Stengel's GPL implementation of LCS-Delta algorithm
 * as it is described in "Introduction to Algorithms", MIT Press, 2001,
 * Second Edition, written by Thomas H. Cormen et. al. It uses dynamic
 * programming to solve the Longest Common Subsequence (LCS) problem.)
 */

#ifndef DIFF_H
#define DIFF_H

#include <QString>

// The following are tags that go at the beginning and end of
// generated diff sections while the diffs are processed
// internally. They are converted to coloured HTML when
// displayed to the user.
static const QString addMarkerStart = QLatin1String("{LokalizeAdd}");
static const QString addMarkerEnd = QLatin1String("{/LokalizeAdd}");
static const QString delMarkerStart = QLatin1String("{LokalizeDel}");
static const QString delMarkerEnd = QLatin1String("{/LokalizeDel}");

/**
 * @short Word-by-word diff algorithm
 *
 * Word-by-word diff algorithm
 *
 * Based on Markus Stengel's GPLv2+ implementation of LCS-Delta algorithm
 * as it is described in "Introduction to Algorithms", MIT Press, 2001, Second Edition, written by Thomas H. Cormen et. al.
 * It uses dynamic programming to solve the Longest Common Subsequence (LCS) problem.
 *
 * This is high-level wrapper
 *
 * @author Nick Shaforostoff <shafff@ukr.net>
 */
enum {
    Html = 1,
};
QString userVisibleWordDiff(const QString &oldString, const QString &newString, const QString &accelRx, const QString &markupRx, int options = 0);

/**
 * This is low-level wrapper used for evaluating translation memory search results
 *
 * You have to explicitly prepend lists with identical strings
 */
QString wordDiff(QStringList s1, QStringList s2);

/**
 * @short Converts curly brace tags into user-readable HTML-coloured diffs
 *
 * This takes a string with Lokalize's diff marker tags e.g.
 * "a{LokalizeAdd}b{/LokalizeAdd}c{LokalizeDel}d{/LokalizeDel}"
 * and replaces the curly brace tags with HTML and CSS that
 * colours the diff using inline CSS. Returns a QString of
 * the coloured diff for use in user-visible areas like the
 * translation memory.
 *
 * @author Finley Watson <fin-w@tutanota.com>
 */
QString diffToHtmlDiff(const QString &diff);

#endif // DIFF_H
