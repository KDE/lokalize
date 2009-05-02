/* **************************************************************************
  This file is part of Lokalize

  wordDiff algorithm adoption and further refinement:
        2007 (C) Nick Shaforostoff <shafff@ukr.net>
  (based on Markus Stengel's GPL implementation of LCS-Delta algorithm as it is described in "Introduction to Algorithms", MIT Press, 2001, Second Edition, written by Thomas H. Cormen et. al. It uses dynamic programming to solve the Longest Common Subsequence (LCS) problem. - http://www.markusstengel.de/text/en/i_4_1_5_3.html)

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
 * http://www.markusstengel.de/text/en/i_4_1_5_3.html)
 *
 * This is high-level wrapper
 *
 * @author Nick Shaforostoff <shafff@ukr.net>
 */
enum {Html=1};
QString userVisibleWordDiff(const QString& oldString,
                 const QString& newString,
                 const QString& accelRx,
                 const QString& markupRx,
                 int options=0);



/**
 * This is low-level wrapper used for evaluating translation memory search results
 *
 * You have to explicitly prepend lists with identical strings
 */
QString wordDiff(QStringList s1, QStringList s2);


#endif // DIFF_H

