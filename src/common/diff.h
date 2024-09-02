/* **************************************************************************
  This file is part of Lokalize

  wordDiff algorithm adoption and further refinement:
        SPDX-FileCopyrightText: 2007 Nick Shaforostoff <shafff@ukr.net>
  SPDX-FileCopyrightText: 2018-2019 Simon Depiets <sdepiets@gmail.com>
  (based on Markus Stengel's GPL implementation of LCS-Delta algorithm as it is described in "Introduction to Algorithms", MIT Press, 2001, Second Edition, written by Thomas H. Cormen et. al. It uses dynamic programming to solve the Longest Common Subsequence (LCS) problem.)

  SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL

************************************************************************** */
#ifndef DIFF_H
#define DIFF_H

#include <QString>


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
enum {Html = 1};
QString userVisibleWordDiff(const QString& oldString,
                            const QString& newString,
                            const QString& accelRx,
                            const QString& markupRx,
                            int options = 0);


/**
 * This is low-level wrapper used for evaluating translation memory search results
 *
 * You have to explicitly prepend lists with identical strings
 */
QString wordDiff(QStringList s1, QStringList s2);


/**
 * @short Converts KBABEL curly brace tags into user-readable HTML-coloured diffs
 *
 * This takes a string with {KBABELADD} and {KBABELDEL} tags
 * e.g. "a{KBABELADD}b{/KBABELADD}c{KBABELDEL}d{/KBABELDEL}"
 * and replaces the curly brace tags with HTML and CSS that
 * colours the diff using inline CSS. Returns a QString of
 * the coloured diff for use in user-visible areas like the
 * translation memory.
 *
 * @author Finley Watson <fin-w@tutanota.com>
 */
QString diffToHtmlDiff(const QString& diff);

#endif // DIFF_H

