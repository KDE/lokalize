/*****************************************************************************
  This file is part of KAider

  SPDX-FileCopyrightText: 2007 Nick Shaforostoff <shafff@ukr.net>
                2018-2019 by Simon Depiets <sdepiets@gmail.com>

  SPDX-License-Identifier: GPL-2.0-or-later

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

**************************************************************************** */


#ifndef WEBQUERYCONTROLLER_H
#define WEBQUERYCONTROLLER_H


#include <QObject>
#include <QQueue>
#include <QRegExp>
class Catalog;
class WebQueryView;
class QTextCodec;
class KJob;

struct CatalogData {
    QString msg;

    //used when the script asks for the same file but with another target language
    // e.g. it easier for machine to translate from russian to ukrainian than from english to ukrainian
    QString msg2;

    WebQueryView* webQueryView;//object to call slots
};



/**
 * uses async http reading.
 *
 * currently one instance per script is used...
 */
class WebQueryController: public /*QThread*/QObject
{
    Q_OBJECT
public:

    explicit WebQueryController(const QString& name, QObject* parent);

public Q_SLOTS:
    void query(const CatalogData& data);

    void slotDownloadResult(KJob*);
Q_SIGNALS:
    void addWebQueryResult(const QString&, const QString&);

//These are for scripts:
Q_SIGNALS:
    void doQuery();
    void postProcess(QString);


//these are for scripts:
public Q_SLOTS:
    QString msg();

    QString filePath();
    void setTwinLangFilePath(QString);
    QString twinLangMsg();


    /**
     * Also may be used to get name of another html file (e.g. of a frame)
     *
     * @param url Lokalize escapes url before downloading
     * @param codec e.g. UTF-8
     * @param rx RegExp that gives result in the first cap.
     *        e.g. "<div id=result_box dir=ltr>([^<]+)</div>"
     */
    void doDownloadAndFilter(QString url, QString codec, QString rx/*, int repeat*/);

    void setResult(QString);






    /*    // If emitted calls the update() scripting function
        // if available.
        void update();*/

// protected:
//     void run();

private:
    QQueue<CatalogData> m_queue;
    bool m_running;
    QString m_name;//of the script file

    //QString urlStr
    QTextCodec* codec;
    QRegExp filter;
//     int repeat;
//     QMutex m_mutex;
};



#endif
