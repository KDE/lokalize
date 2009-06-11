/*****************************************************************************
  This file is part of KAider

  Copyright (C) 2007	  by Nick Shaforostoff <shafff@ukr.net>

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

**************************************************************************** */

#include "webquerycontroller.h"
#include <QTextCodec>
#include "catalog.h"
#include "webqueryview.h"

#include <QUrl>
// #include <kio/netaccess.h>
#include <kio/jobclasses.h>
#include <kio/job.h>
#include <kdebug.h>


WebQueryController::WebQueryController(const QString& name, QObject* parent)
//     : QThread(parent)
    : QObject(parent)
    , m_running(false)
    , m_name(name)
{
}

void WebQueryController::query(const CatalogData& data)
{
    m_queue.enqueue(data);
    if(!m_running)
    {
        m_running=true;
        emit doQuery();
    }
}



QString WebQueryController::msg()
{
    return m_queue.head().msg;
}

QString WebQueryController::filePath()
{
    return QString();
}
void WebQueryController::setTwinLangFilePath(QString)
{

}

QString WebQueryController::twinLangMsg()
{
    return QString();
}


void WebQueryController::doDownloadAndFilter(QString urlStr, QString _codec, QString rx/*, int rep*/)
{
    QString result;
    QUrl url;
    url.setUrl(urlStr);

    kWarning()<<"_real url: "<<url.toString();
    KIO::StoredTransferJob* readJob = KIO::storedGet(url, KIO::NoReload, KIO::HideProgressInfo);
    connect(readJob,SIGNAL(result(KJob*)),this,SLOT(slotDownloadResult(KJob*)));
    readJob->setAutoDelete(false);//HACK HACK HACK

    codec=QTextCodec::codecForName(_codec.toUtf8());
    filter=QRegExp(rx);
}

void WebQueryController::slotDownloadResult(KJob* job)
{
    m_running=false;
    if ( job->error() )
    {
        m_queue.dequeue();
        delete job;
        return;
    }

    QTextStream stream(static_cast<KIO::StoredTransferJob*>(job)->data());
    stream.setCodec(codec);
    if (filter.indexIn(stream.readAll())!=-1)
    {
        emit postProcess(filter.cap(1));
        //kWarning()<<result;
    }
    else
        m_queue.dequeue();

    delete job;
}


void WebQueryController::setResult(QString result)
{
    //webQueryView may be deleted before we get result...
    WebQueryView* a=m_queue.dequeue().webQueryView;
    connect (this,SIGNAL(addWebQueryResult(const QString&,const QString&)),
             a,SLOT(addWebQueryResult(const QString&,const QString&)));
    emit addWebQueryResult(m_name,result);
    disconnect (this,SIGNAL(addWebQueryResult(const QString&,const QString&)),
             a,SLOT(addWebQueryResult(const QString&,const QString&)));

    if(!m_queue.isEmpty())
    {
        m_running=true;
        emit doQuery();
    }

}


#include "webquerycontroller.moc"

