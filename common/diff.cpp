/* **************************************************************************
  This file is part of KAider

  wordDiff algorithm adoption (C) Nick Shaforostoff <shafff@ukr.net>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

  In addition, as a special exception, the copyright holders give
  permission to link the code of this program with any edition of
  the Qt library by Trolltech AS, Norway (or with modified versions
  of Qt that use the same license as Qt), and distribute linked
  combinations including the two.  You must obey the GNU General
  Public License in all respects for all of the code used other than
  Qt. If you modify this file, you may extend this exception to
  your version of the file, but you are not obligated to do so.  If
  you do not wish to do so, delete this exception statement from
  your version.

************************************************************************** */

#include "diff.h"

#include <kdebug.h>


LCSprinter::LCSprinter(const QStringList &s_1, const QStringList &s_2, QVector<LCSMarker> *b_, const uint nT_, uint index):s1(s_1),s2(s_2),nT(nT_),b(b_)
{
    it1=s1.begin();
    it2=s2.begin();
    printLCS(index);
}

void LCSprinter::printLCS(uint index)
{
    //fprintf(stderr,"%2d. %2d. %2d. %2d\n",(uint)(*b)[index],nT,index%nT, index);
    if (index % nT == 0 || index < nT)
    {
    //original LCS algo doesnt have to deal with ins before first common
        uint bound = index%nT;
        for (index=0; index<bound; ++index)
        {
            resultString.append("<KBABELADD>");
//            kWarning() << "add1 " << *it2 << endl;
            resultString.append(*it2);
            ++it2;
            resultString.append("</KBABELADD>");
        }

        return;
    }

    if (ARROW_UP_LEFT == (*b)[index])
    {
        printLCS(index-nT-1);
        if (it1!=s1.end())
        {
            resultString.append(*it1);
//             kWarning() << "upleft " << *it1 << endl;
        }
            ++it1;
        ++it2;
        return;
    }
    else if (ARROW_UP == (*b)[index])
    {
        printLCS(index-nT);
//         if (it1!=s1.end())
        {
//             kWarning()<<"APPENDDEL "<<*it1<<endl;
            resultString.append("<KBABELDEL>");
            resultString.append(*it1);
//             kWarning() << "del " << *it1 << endl;
            ++it1;
            resultString.append("</KBABELDEL>");
        }
        return;
    }
    else
    {
        printLCS(index-1);
//         kWarning()<<"APPENDADD "<<*it2<<endl;
        resultString.append("<KBABELADD>");
        resultString.append(*it2);
//         kWarning() << "add " << *it2 << endl;
        ++it2;
        resultString.append("</KBABELADD>");
        return;
    }

}



QString wordDiff(const QString& str1, const QString& str2)
{
    //separate punctuation marks etc from words as _only_ they may have changed
    QStringList s1, s2;

    uint i=0;
    uint j=0;
    uint l1=str1.length();
    uint l2=str2.length();
    QString temp;
    temp.reserve(16);

    while ( i<l1 )
    {
        if (i+1<l1 && str1.at(i)==' ' && str1.at(i+1).isLetterOrNumber())
        {
            temp = ' ';
            ++i;
        }
        else
        {
            while ( i<l1 && !str1.at(i).isLetterOrNumber() )
                s1.append(QString(str1.at(i++)));
        }

        while ( i<l1 && str1.at(i).isLetterOrNumber())
            temp += str1.at(i++);

        if (!temp.isEmpty())
        {
/*            if (i<l1 && str1.at(i)==' ' && (i+1==l1 || str1.at(i+1).isLetter()))
            {
                temp += ' ';
                ++i;
            }*/
            s1.append(temp);
            temp.clear();
        };

/*        while ( i<l1 && !str1.at(i).isLetter() )
        {
            s1.append(QString(str1.at(i++)));
        }*/
    }

    i=0;
    while ( i<l2 )
    {
        if (i+1<l2 && str2.at(i)==' ' && (str2.at(i+1).isLetterOrNumber()))
        {
            temp = ' ';
            ++i;
        }
        else
        {
            while ( i<l2 && !str2.at(i).isLetterOrNumber() )
                s2.append(QString(str2.at(i++)));
        }
        while ( i<l2 && str2.at(i).isLetterOrNumber() )
            temp += str2.at(i++);
        if (!temp.isEmpty())
        {
/*            if (i<l2 && str2.at(i)==' ' && (i+1==l2 || str2.at(i+1).isLetter()))
            {
                temp += ' ';
                ++i;
            }*/
            s2.append(temp);
            temp.clear();
        };

    }

    uint mX = s1.count();
    uint nY = s2.count();

    uint mT = mX+1;
    uint nT = nY+1;

    QVector<LCSMarker> b(mT*nT, NOTHING);
    QVector<uint> c(mT*nT, 0);


// calculate the LCS
    b[0] = FINAL;
    uint index_cache = 0;
    QStringList::iterator it1, it2;

    for (i=1, it1 = s1.begin(); i<mT; ++i, ++it1)
    {
        for (j=1, it2 = s2.begin(); j<nT; ++j, ++it2)
        {
            index_cache = i*nT+j;
            if ((*it1)==(*it2))
            {
                c[index_cache] = c[index_cache-nT-1] + 1;
                b[index_cache] = ARROW_UP_LEFT;
            }
            else if (c[index_cache-nT] >= c[index_cache-1])
            {
                c[index_cache] = c[index_cache-nT];
                b[index_cache] = ARROW_UP;
            }
            else
            {
                c[index_cache] = c[index_cache-1];
                b[index_cache] = ARROW_LEFT;
            }
        }
    }

    c.clear();

 
    LCSprinter printer(s1, s2, &b, nT, index_cache);


    return printer.getString();
}
