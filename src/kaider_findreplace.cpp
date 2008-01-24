/* ****************************************************************************
  This file is part of KAider

  Copyright (C) 2007 by Nick Shaforostoff <shafff@ukr.net>

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

#include "kaider.h"
#include "kaiderview.h"
#include "cataloglistview.h"
#include "catalog.h"
#include "pos.h"
#include "cmd.h"
#include "prefs_lokalize.h"
#include "ui_kaider_findextension.h"


#include <kglobal.h>
#include <klocale.h>
#include <kdebug.h>
#include <kurl.h>

#include <kprogressdialog.h>

#include <kreplacedialog.h>
#include <kreplace.h>

#include <sonnet/backgroundchecker.h>

#include <QTimer>


//  #include "global.h"

//#define FIND_IGNOREACCELS 2048
//#define FIND_SKIPTAGS 4096
#define FIND_IGNOREACCELS ui_findExtension->m_ignoreAccelMarks->isChecked()
#define FIND_SKIPTAGS ui_findExtension->m_skipTags->isChecked()
#define REPLACE_IGNOREACCELS ui_replaceExtension->m_ignoreAccelMarks->isChecked()

void KAider::deleteUiSetupers()
{
    delete ui_findExtension;
    delete ui_replaceExtension;
}

static void cleanUpIfMultiple(KAider* th,
                              KUrl::List& list,
                              int& pos,
                              KFindDialog* dia)
{
    if (!list.isEmpty())
    {
        if (!th->isVisible())
            th->deleteLater();
        list.clear();
        pos=-1;
        dia->setHasCursor(true);
    }
}

void KAider::find()
{
    if( !_findDialog )
    {
        _findDialog = new KFindDialog(this);
        if( !ui_findExtension ) //actually, we don't need this check...
            ui_findExtension = new Ui_findExtension;
        ui_findExtension->setupUi(_findDialog->findExtension());
        _findDialog->setHasSelection(false);
    }

    /////
    if (m_searchFilesPos!=-1)
    {
        //reset find in files state
        m_searchFilesPos=-1;
        m_searchFiles.clear();
        _findDialog->setHasCursor(true);
    }
    else if (!m_searchFiles.isEmpty())
        _findDialog->setHasCursor(false);
    /////

    QString sel(m_view->selection());
    if (!(sel.isEmpty()&&m_view->selectionMsgId().isEmpty()))
    {
        if (sel.isEmpty())
            sel=m_view->selectionMsgId();
        if (FIND_IGNOREACCELS)
            sel.remove('&');
            _findDialog->setPattern(sel);
    }

    if ( _findDialog->exec() != QDialog::Accepted )
    {
        cleanUpIfMultiple(this,
                          m_searchFiles,
                          m_searchFilesPos,
                          _findDialog);
        return;
    }
    //HACK dunno why!      //     kWarning() << "pat " << _findDialog->findHistory();
     _findDialog->setPattern(_findDialog->findHistory().first());

    if (_find)
    {
        _find->resetCounts();
        _find->setPattern(_findDialog->pattern());
        _find->setOptions(_findDialog->options()
//                          +ui_findExtension->m_ignoreAccelMarks->isChecked()?FIND_IGNOREACCELS:0
//                          +ui_findExtension->m_skipTags->isChecked()?FIND_SKIPTAGS:0
                         );

    }
    else // This creates a find-next-prompt dialog if needed.
    {
        _find = new KFind(_findDialog->pattern(),_findDialog->options(),this,_findDialog);
        connect(_find,SIGNAL(highlight(const QString&,int,int)),
                this, SLOT(highlightFound(const QString &,int,int)) );
        connect(_find,SIGNAL(findNext()),this,SLOT(findNext()));
        _find->closeFindNextDialog();
    }

    DocPosition pos;
    if (_find->options() & KFind::FromCursor)
        pos=_currentPos;
    else
    {
        if (!determineStartingPos(_find,m_searchFiles,m_searchFilesPos,pos))
        {
            cleanUpIfMultiple(this,
                              m_searchFiles,
                              m_searchFilesPos,
                              _findDialog);
            return;
        }
    }

    findNext(pos);
}

bool KAider::determineStartingPos(KFind* find,
                                  const KUrl::List& filesList,//search or replace files
                                  int& filesPos,
                                  DocPosition& pos)
{
    if (find->options() & KFind::FindBackwards)
    {
        /////
        if (!filesList.isEmpty())
        {
            filesPos=filesList.size()-1;
            if (filesList.size()!=1)
            {
                m_updateView=false;
            }
            if ((find==_replace&&!fileSave())
               ||!fileOpen(filesList.at(filesPos)))
                return false;
            if (filesList.size()!=1)
            {
                m_updateView=true;
            }
        }
        /////

        pos.entry=_catalog->numberOfEntries()-1;
        pos.form=
                (_catalog->pluralFormType(pos.entry)==Gettext)?
                _catalog->numberOfPluralForms()-1:0;

    }
    else
    {
        /////
        if (!filesList.isEmpty())
        {
            filesPos=0;
            if (filesList.size()!=1)
            {
                m_updateView=false;
            }
            if ((find==_replace&&!fileSave())
               ||!fileOpen(filesList.at(filesPos)))
                return false;
            if (filesList.size()!=1)
            {
                m_updateView=true;
            }
        }
        /////

        pos.entry=0;
        pos.form=0;

    }
    return true;
}

// void KAider::initProgressDia()
// {
//     
// }

void KAider::findNext(const DocPosition& startingPos)
{
    Catalog& catalog=*_catalog;
    KFind& find=*_find;

    if (catalog.numberOfEntries()<=startingPos.entry)
        return;//for the case when app wasn't able to process event before file close

    bool anotherEntry=_searchingPos.entry!=_currentPos.entry;
    _searchingPos=startingPos;

    if (anotherEntry)
        _searchingPos.offset=0;

    /////
    if (!m_searchFiles.isEmpty()
         &&m_searchFilesPos>=0
         &&m_searchFilesPos<m_searchFiles.size())
    {

        m_catalogTreeView->setUpdatesEnabled(false);
        //it is a qobject so it certainly will be deleted when needed
        if (!m_progressDialog)
        {
            m_progressDialog=new KProgressDialog(this,
                                         i18nc("@title:window","Scanning Files...")
                                     //i18nc("@title:window","Searching in Files"),
                                        );
            m_progressDialog->setAllowCancel(false);
            m_progressDialog->showCancelButton(false);
        }
        m_progressDialog->progressBar()->setRange(0,m_searchFiles.size()-1);
        m_progressDialog->progressBar()->setValue(m_searchFilesPos);
        m_progressDialog->show();

    }
    /////


    QRegExp rx("[^(\\\\n)>]\n");
    QTime a;a.start();
    //_searchingPos.part=Msgid;
    bool ignoreaccels=FIND_IGNOREACCELS;
    int flag=1;
//     int offset=_searchingPos.offset;
    while (flag)
    {

        flag=0;
        KFind::Result res = KFind::NoMatch;
        while (true)
        {
            if (find.needData()||anotherEntry)
            {
                anotherEntry=false;

                QString data;
                if (_searchingPos.part==Msgid)
                {
                    data=catalog.msgid(_searchingPos)/*,offset*/;

                    //unwrap bc kaiderview does that too
                    int p=0;
                    while ((p=rx.indexIn(data))!=-1)
                        data.remove(p+1,1);
                }
                else
                    data=catalog.msgstr(_searchingPos)/*,offset*/;

                if (ignoreaccels)
                    data.remove('&');
                find.setData(data);
            }

            res = find.find();
            //offset=-1;
            if (res!=KFind::NoMatch)
                break;

            if (!(
                  (find.options()&KFind::FindBackwards)?
                                switchPrev(_catalog,_searchingPos,true):
                                switchNext(_catalog,_searchingPos,true)
                 ))
                break;
        }

        if (res==KFind::NoMatch)
        {
            /////
            if (!m_searchFiles.isEmpty())
            {
                bool end=false;
                bool last=false;
                if (find.options() & KFind::FindBackwards)
                {
                    end=(--m_searchFilesPos<0);
                    last=(m_searchFilesPos==0);
                }
                else
                {
                    end=(++m_searchFilesPos==m_searchFiles.size());
                    last=(m_searchFilesPos==m_searchFiles.size()-1);
                }
                if (KDE_ISLIKELY(m_progressDialog))
                    m_progressDialog->progressBar()->setValue(m_searchFilesPos);

                if (KDE_ISLIKELY(!end))
                {
                    if (!last)
                        m_updateView=false;
                    if (m_searchFilesPos<m_searchFiles.size()&&fileOpen(m_searchFiles.at(m_searchFilesPos)))
                    {
                        if (find.options() & KFind::FindBackwards)
                        {
                            DocPosition pos;
                            pos.entry=catalog.numberOfEntries()-1;
                            pos.form=(catalog.pluralFormType(pos.entry)==Gettext)?
                                catalog.numberOfPluralForms()-1:0;
                            //_searchingPos=pos;
                            gotoEntry(pos);
                        }
                        //flag=1;

                        if (!last)
                            m_updateView=true;
                        //continue;
                        QTimer::singleShot(0,this,SLOT(findNext()));
                        //hideDia=false;

                        kWarning()<<"searchink         TIME "<<a.elapsed();
                        return;
                    }
                    else
                        cleanUpIfMultiple(this,
                                  m_searchFiles,
                                  m_searchFilesPos,
                                  _findDialog);

                }
            }
            if(m_progressDialog)
                m_progressDialog->hide();
            /////

            //file-wide search, or end of project-wide search
            if(find.shouldRestart(true,true)
              &&determineStartingPos(_find,m_searchFiles,m_searchFilesPos,_searchingPos))
            {
                flag=1;
            }
            /////
            else 
                cleanUpIfMultiple(this,
                                  m_searchFiles,
                                  m_searchFilesPos,
                                  _findDialog);
            /////
            find.resetCounts();
        }
    }

    if(m_progressDialog)
        m_progressDialog->hide();

    m_catalogTreeView->setUpdatesEnabled(true);
    kWarning()<<"searchink         TIME "<<a.elapsed();
}

void KAider::findNext()
{
    if (_find)
        findNext(_currentPos);
    else
        find();

}

void KAider::findPrev()
{

    if (_find)
    {
        _find->setOptions(_find->options() ^ KFind::FindBackwards);
        findNext(_currentPos);
    }
    else
    {
        find();
    }

}

void KAider::highlightFound(const QString &,int matchingIndex,int matchedLength)
{
    show();

    if (FIND_IGNOREACCELS)
    {
        QString data;
        if (_searchingPos.part==Msgid)
            data=_catalog->msgid(_searchingPos);
        else
            data=_catalog->msgstr(_searchingPos);
        int i=0;
        for (;i<matchingIndex;++i)
            if (data.at(i)=='&')
                ++matchingIndex;

        int limit=matchingIndex+matchedLength;
        for (i=matchingIndex;i<limit;++i)
            if (data.at(i)=='&')
            {
                ++matchedLength;
                limit=qMin(data.size(),matchingIndex+matchedLength);
            }
    }

    _searchingPos.offset=matchingIndex;
    gotoEntry(_searchingPos,matchedLength);

}

void KAider::replace()
{
    if( !_replaceDialog )
    {
        _replaceDialog = new KReplaceDialog(this);
        if( !ui_replaceExtension ) //we actually don't need this check...
            ui_replaceExtension = new Ui_findExtension;
        ui_replaceExtension->setupUi(_replaceDialog->replaceExtension());
        _replaceDialog->setHasSelection(false);
    }

    /////
    if (m_replaceFilesPos!=-1)
    {
        //reset find in files state
        m_replaceFilesPos=-1;
        m_replaceFiles.clear();
        _replaceDialog->setHasCursor(true);
    }
    else if (!m_replaceFiles.isEmpty())
        _replaceDialog->setHasCursor(false);

    /////

    if (!m_view->selection().isEmpty())
    {
        if (REPLACE_IGNOREACCELS)
        {
            QString tmp(m_view->selection());
            tmp.remove('&');
            _replaceDialog->setPattern(tmp);
        }
        else
            _replaceDialog->setPattern(m_view->selection());
    }


    if ( _replaceDialog->exec() != QDialog::Accepted )
    {
        cleanUpIfMultiple(this,
                          m_replaceFiles,
                          m_replaceFilesPos,
                          _replaceDialog);
        return;
    }

//HACK dunno why!
    _replaceDialog->setPattern(_replaceDialog->findHistory().first());


    if (_replace)
    {
        delete _replace;
        _replace=0;
    }

    // This creates a find-next-prompt dialog if needed.
    {
        _replace = new KReplace(_replaceDialog->pattern(),_replaceDialog->replacement(),_replaceDialog->options(),this,_replaceDialog);
        connect(_replace,SIGNAL(highlight(const QString&,int,int)),
                this,SLOT(highlightFound_(const QString&,int,int)));
        connect(_replace,SIGNAL(findNext()),this,SLOT(replaceNext()));
        connect(_replace,SIGNAL(replace(const QString&,int,int,int)),
                this,SLOT(doReplace(const QString&,int,int,int)));
//         _replace->closeReplaceNextDialog();
    }
//     else
//     {
//         _replace->resetCounts();
//         _replace->setPattern(_replaceDialog->pattern());
//         _replace->setOptions(_replaceDialog->options());
//     }

    //_catalog->beginMacro(i18nc("@item Undo action item","Replace"));
    m_doReplaceCalled=false;

    if (_replace->options() & KFind::FromCursor)
    {
        replaceNext(_currentPos);
    }
    else
    {
        DocPosition pos;
        if (!determineStartingPos(_replace,m_replaceFiles,m_replaceFilesPos,pos))
        {
            cleanUpIfMultiple(this,
                              m_replaceFiles,
                              m_replaceFilesPos,
                              _replaceDialog);
            return;
        }
        replaceNext(pos);
    }

//     kWarning() << "END";
}


void KAider::replaceNext(const DocPosition& startingPos)
{
    bool anotherEntry=_replacingPos.entry!=_replacingPos.entry;
    _replacingPos=startingPos;

    if (anotherEntry)
        _replacingPos.offset=0;

    /////
    if (!m_replaceFiles.isEmpty()
         &&m_replaceFilesPos>=0
         &&m_replaceFilesPos<m_replaceFiles.size())
    {
        m_catalogTreeView->setUpdatesEnabled(false);
        //it is a qobject so it certainly will be deleted when needed
        if (!m_progressDialog)
        {
            m_progressDialog=new KProgressDialog(this,
                                         i18nc("@title:window","Scanning Files...")
                                     //i18nc("@title:window","Searching in Files"),
                                        );
            m_progressDialog->setAllowCancel(false);
            m_progressDialog->showCancelButton(false);
        }
        m_progressDialog->progressBar()->setRange(0,m_replaceFiles.size()-1);
        m_progressDialog->progressBar()->setValue(m_replaceFilesPos);
        m_progressDialog->show();
    }
    /////


    int flag=1;
//     int offset=_replacingPos.offset;
    while (flag)
    {
        flag=0;
        KFind::Result res=KFind::NoMatch;
        while (1)
        {
            if (_replace->needData()||anotherEntry)
            {
                anotherEntry=false;

                if (REPLACE_IGNOREACCELS)
                {
                    QString data(_catalog->msgstr(_replacingPos));
                    data.remove('&');
                    _replace->setData(data);
                }
                else
                    _replace->setData( _catalog->msgstr(_replacingPos));
            }
            res = _replace->replace();
//             offset=-1;
            if (res!=KFind::NoMatch)
                break;

            if (!(
                  (_replace->options()&KFind::FindBackwards)?
                                switchPrev(_catalog,_replacingPos):
                                switchNext(_catalog,_replacingPos)
                 ))
                break;
        }

        if (res==KFind::NoMatch)
        {
            /////
            if (!m_replaceFiles.isEmpty())
            {
                bool end=false;
                bool last=false;
                if (_replace->options() & KFind::FindBackwards)
                {
                    end=(--m_replaceFilesPos<0);
                    last=(m_replaceFilesPos==0);
                }
                else
                {
                    end=(++m_replaceFilesPos==m_replaceFiles.size());
                    last=(m_replaceFilesPos==m_replaceFiles.size()-1);
                }
                if (KDE_ISLIKELY(m_progressDialog))
                    m_progressDialog->progressBar()->setValue(m_replaceFilesPos);

                if (KDE_ISLIKELY(!end))
                {
                    if (!last)
                    {
                        m_updateView=false;
                    }
                    if (!fileSave()
                       ||fileOpen(m_replaceFiles.at(m_replaceFilesPos)))
                    {
                        cleanUpIfMultiple(this,
                                          m_replaceFiles,
                                          m_replaceFilesPos,
                                          _replaceDialog);
                        return;
                    }
                    if (_replace->options() & KFind::FindBackwards)
                    {
                        DocPosition pos;
                        pos.entry=_catalog->numberOfEntries()-1;
                        pos.form=(_catalog->pluralFormType(pos.entry)==Gettext)?
                            _catalog->numberOfPluralForms()-1:0;
                        //_searchingPos=pos;
                        gotoEntry(pos);
                    }
                    //flag=1;
                    //determineStartingPos(_searchingPos);

                    if (!last)
                    {
                        m_updateView=true;
                    }
                    //continue;
                    QTimer::singleShot(0,this,SLOT(replaceNext()));
                    //hideDia=false;
                    return;
                }
            }
            if(m_progressDialog)
                m_progressDialog->hide();
            /////

            if((_replace->options()&KFind::FromCursor)
                &&_replace->shouldRestart(true)
                &&determineStartingPos(_replace,m_replaceFiles,m_replaceFilesPos,_replacingPos))
            {
                flag=1;
            }
            else
            {
                cleanUpIfMultiple(this,
                                  m_replaceFiles,
                                  m_replaceFilesPos,
                                  _replaceDialog);

//HACK avoid crash
//                 _replace->closeReplaceNextDialog();

                if(!(_replace->options() & KFind::FromCursor))
                     _replace->displayFinalDialog();

                if(m_doReplaceCalled)
                {
                    m_doReplaceCalled=false;
                    _catalog->endMacro();
                }
            }
            _replace->resetCounts();
        }
    }

    if(m_progressDialog)
        m_progressDialog->hide();

    m_catalogTreeView->setUpdatesEnabled(true);
}

void KAider::replaceNext()
{
    replaceNext(_currentPos);
//     kWarning() << "END";
}

void KAider::highlightFound_(const QString &,int matchingIndex,int matchedLength)
{
    show();

    if (REPLACE_IGNOREACCELS)
    {
        QString data;
        if (_replacingPos.part==Msgid)
            data=_catalog->msgid(_replacingPos);
        else
            data=_catalog->msgstr(_replacingPos);
        int i=0;
        for (;i<matchingIndex;++i)
            if (data.at(i)=='&')
                ++matchingIndex;

        int limit=matchingIndex+matchedLength;
        for (i=matchingIndex;i<matchingIndex+matchedLength;++i)
            if (data.at(i)=='&')
            {
                ++matchedLength;
                limit=qMin(data.size(),matchingIndex+matchedLength);
            }
    }

    _replacingPos.offset=matchingIndex;
    gotoEntry(_replacingPos,matchedLength);

}


void KAider::doReplace(const QString &newStr,int offset,int newLen,int remLen)
{
    if(!m_doReplaceCalled)
    {
        m_doReplaceCalled=true;
        _catalog->beginMacro(i18nc("@item Undo action item","Replace"));
    }
    QString oldStr(_catalog->msgstr(_replacingPos));

    if (REPLACE_IGNOREACCELS)
    {
        int i=0;
        for (;i<offset;++i)
            if (oldStr.at(i)=='&')
                ++offset;

        int limit=offset+remLen;
        for (i=offset;i<limit;++i)
            if (oldStr.at(i)=='&')
            {
                ++remLen;
                limit=qMin(oldStr.size(),offset+remLen);
            }
    }

    QString tmp(oldStr.mid(offset,remLen));
//     if (tmp==_replaceDialog->pattern())
//         tmp=_replaceDialog->pattern();
    DocPosition pos=_replacingPos;
    pos.offset=offset;
    _catalog->push(new DelTextCmd(_catalog,pos,tmp));

    if (newLen)
    {
        tmp=newStr.mid(offset,newLen);
        //does it save memory?
/*        if (tmp==_replaceDialog->replacement())
            tmp=_replaceDialog->replacement();*/
        _catalog->push(new InsTextCmd(_catalog,pos,tmp));
    }

    if (pos.entry==_currentEntry)
    {
        pos.offset+=newLen;
        m_view->gotoEntry(pos);
    }
}




#undef FIND_IGNOREACCELS
#undef FIND_SKIPTAGS
#undef REPLACE_IGNOREACCELS









void KAider::spellcheck()
{
    if (!m_sonnetDialog)
    {
        m_sonnetDialog=new Sonnet::Dialog(
            new Sonnet::BackgroundChecker( this ),
            this );
        connect(m_sonnetDialog,SIGNAL(done(const QString&)),this,SLOT(spellcheckNext()));
        connect(m_sonnetDialog,SIGNAL(replace(const QString&,int,const QString&)),
            this,SLOT(spellcheckReplace(const QString&,int,const QString&)));
        connect(m_sonnetDialog,SIGNAL(misspelling(const QString&,int)),
            this,SLOT(spellcheckShow(const QString&,int)));
        connect(m_sonnetDialog,SIGNAL(stop()),this,SLOT(spellcheckStop()));
        connect(m_sonnetDialog,SIGNAL(cancel()),this,SLOT(spellcheckCancel()));
    }

    if (!m_spellcheckFiles.isEmpty()
         &&m_spellcheckFilesPos==-1)
    {
        if (!fileOpen(m_spellcheckFiles.first()))
        {
            m_spellcheckFiles.clear();
            m_spellcheckFilesPos=-1;
            return;
        }
        m_spellcheckFilesPos=0;
    }

    if (!m_view->selection().isEmpty())
        m_sonnetDialog->setBuffer(m_view->selection());
    else
        m_sonnetDialog->setBuffer(_catalog->msgstr(_currentPos));

    _spellcheckPos=_currentPos;
    _spellcheckStop=false;
    //_catalog->beginMacro(i18n("Spellcheck"));
    _spellcheckStartUndoIndex=_catalog->index();
    m_sonnetDialog->show();

}


void KAider::spellcheckNext()
{
    //DocPosition pos=_spellcheckPos;

    if (!_spellcheckStop && switchNext(_catalog,_spellcheckPos))
    {
        // HACK actually workaround
        while (_catalog->msgstr(_spellcheckPos).isEmpty())
        {
            if (!switchNext(_catalog,_spellcheckPos))
            {
                if (m_spellcheckFiles.isEmpty())
                    return;
                if (++m_spellcheckFilesPos==m_spellcheckFiles.size()
                    ||!fileSave()
                    ||!fileOpen(m_spellcheckFiles.at(m_spellcheckFilesPos)))
                {
                    m_spellcheckFiles.clear();
                    m_spellcheckFilesPos=-1;
                    if (!isVisible())
                        deleteLater();
                    return;
                }
                QTimer::singleShot(0,this,SLOT(spellcheck()));
                return;
            }
        m_sonnetDialog->setBuffer( _catalog->msgstr(_spellcheckPos) );
//             kWarning() << "spellcheckNext a"<<_catalog->msgstr(_spellcheckPos);

        }
    }
}

void KAider::spellcheckStop()
{
    _spellcheckStop=true;
}

void KAider::spellcheckCancel()
{
    _catalog->setIndex(_spellcheckStartUndoIndex);
    gotoEntry(_spellcheckPos);
}

void KAider::spellcheckShow(const QString &word, int offset)
{
    show();

//     kWarning() << "spellcheckShw "<<word;
    DocPosition pos=_spellcheckPos;
    pos.offset=offset;
    gotoEntry(pos,word.length());
//     kWarning() << "spellcheckShw "<<word;
}

void KAider::spellcheckReplace(const QString &oldWord, int offset, const QString &newWord)
{
    DocPosition pos=_spellcheckPos;
    pos.offset=offset;

    _catalog->push(new DelTextCmd(_catalog,pos,oldWord));
    _catalog->push(new InsTextCmd(_catalog,pos,newWord));
    gotoEntry(pos,newWord.length());
}












void KAider::findInFiles(const KUrl::List& list)
{
    m_searchFiles=list;
    m_searchFilesPos=-1;

#if 0
this is nice, but too complex :)
    KUrl::List::iterator it(m_searchFiles.begin());
    while (it!=m_searchFiles.end())
    {
        if (_catalog->url()==*it)
            break;
        ++it;
    }

    if (it!=m_searchFiles.end())
        m_searchFiles.erase(it);
    else if (!m_searchFiles.isEmpty())
        fileOpen(m_searchFiles.takeFirst());
#endif

    QTimer::singleShot(0,this,SLOT(find()));
}


void KAider::replaceInFiles(const KUrl::List& list)
{
    m_replaceFiles=list;
    m_replaceFilesPos=-1;

    QTimer::singleShot(0,this,SLOT(replace()));
}


void KAider::spellcheckFiles(const KUrl::List& list)
{
    m_spellcheckFiles=list;
    m_spellcheckFilesPos=-1;

    QTimer::singleShot(0,this,SLOT(spellcheck()));
}





