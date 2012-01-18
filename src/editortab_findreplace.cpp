/* ****************************************************************************
  This file is part of Lokalize

  Copyright (C) 2007-2009 by Nick Shaforostoff <shafff@ukr.net>

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

**************************************************************************** */

#include "editortab.h"
#include "editorview.h"
#include "catalog.h"
#include "pos.h"
#include "cmd.h"
#include "project.h"
#include "prefs_lokalize.h"
#include "ui_kaider_findextension.h"
#include "stemming.h"


#include <kglobal.h>
#include <kmessagebox.h>
#include <klocale.h>
#include <kdebug.h>
#include <kurl.h>

#include <kprogressdialog.h>

#include <kreplacedialog.h>
#include <kreplace.h>

#include <sonnet/backgroundchecker.h>
#include <sonnet/dialog.h>

#include <QTimer>
#include <QPointer>


#define IGNOREACCELS KFind::MinimumUserOption
#define INCLUDENOTES KFind::MinimumUserOption*2

static long makeOptions(long options, const Ui_findExtension* ui_findExtension)
{
    return options
              +IGNOREACCELS*ui_findExtension->m_ignoreAccelMarks->isChecked()
              +INCLUDENOTES*ui_findExtension->m_notes->isChecked();
    //bool skipMarkup(){return ui_findExtension->m_skipTags->isChecked();}
}

class EntryFindDialog: public KFindDialog
{
public:
    EntryFindDialog(QWidget* parent);
    ~EntryFindDialog();
    long options() const{return makeOptions(KFindDialog::options(),ui_findExtension);}
    static EntryFindDialog* instance(QWidget* parent=0);
private:
    static QPointer<EntryFindDialog> _instance;
    static void cleanup(){delete EntryFindDialog::_instance;}
private:
    Ui_findExtension* ui_findExtension;
};

QPointer<EntryFindDialog> EntryFindDialog::_instance=0;
EntryFindDialog* EntryFindDialog::instance(QWidget* parent)
{
    if (_instance==0 )
    {
        _instance=new EntryFindDialog(parent);
        qAddPostRoutine(EntryFindDialog::cleanup);
    }
    return _instance;
}

EntryFindDialog::EntryFindDialog(QWidget* parent)
 : KFindDialog(parent)
 , ui_findExtension(new Ui_findExtension)
{
    ui_findExtension->setupUi(findExtension());
    setHasSelection(false);

    KConfig config;
    KConfigGroup stateGroup(&config,"FindReplace");
    setOptions(stateGroup.readEntry("FindOptions",(qlonglong)0));
    setFindHistory(stateGroup.readEntry("FindHistory",QStringList()));
}

EntryFindDialog::~EntryFindDialog()
{
    KConfig config;
    KConfigGroup stateGroup(&config,"FindReplace");
    stateGroup.writeEntry("FindOptions",(qlonglong)options());
    stateGroup.writeEntry("FindHistory",findHistory());

    delete ui_findExtension;
}

//BEGIN EntryReplaceDialog
class EntryReplaceDialog: public KReplaceDialog
{
public:
    EntryReplaceDialog(QWidget* parent);
    ~EntryReplaceDialog();
    long options() const{return makeOptions(KReplaceDialog::options(),ui_findExtension);}
    static EntryReplaceDialog* instance(QWidget* parent=0);
private:
    static QPointer<EntryReplaceDialog> _instance;
    static void cleanup(){delete EntryReplaceDialog::_instance;}
private:
    Ui_findExtension* ui_findExtension;
};

QPointer<EntryReplaceDialog> EntryReplaceDialog::_instance=0;
EntryReplaceDialog* EntryReplaceDialog::instance(QWidget* parent)
{
    if (_instance==0 )
    {
        _instance=new EntryReplaceDialog(parent);
        qAddPostRoutine(EntryReplaceDialog::cleanup);
    }
    return _instance;
}

EntryReplaceDialog::EntryReplaceDialog(QWidget* parent)
 : KReplaceDialog(parent)
 , ui_findExtension(new Ui_findExtension)
{
    ui_findExtension->setupUi(findExtension());
    //ui_findExtension->m_notes->hide();
    setHasSelection(false);

    KConfig config;
    KConfigGroup stateGroup(&config,"FindReplace");
    setOptions(stateGroup.readEntry("ReplaceOptions",(qlonglong)0));
    setFindHistory(stateGroup.readEntry("ReplacePatternHistory",QStringList()));
    setReplacementHistory(stateGroup.readEntry("ReplacementHistory",QStringList()));
}

EntryReplaceDialog::~EntryReplaceDialog()
{
    KConfig config;
    KConfigGroup stateGroup(&config,"FindReplace");
    stateGroup.writeEntry("ReplaceOptions",(qlonglong)options());
    stateGroup.writeEntry("ReplacePatternHistory",findHistory());
    stateGroup.writeEntry("ReplacementHistory",replacementHistory());

    delete ui_findExtension;
}
//END EntryReplaceDialog

//TODO &amp;, &nbsp;
static void calcOffsetWithAccels(const QString& data, int& offset, int& length)
{
    int i=0;
    for (;i<offset;++i)
        if (KDE_ISUNLIKELY( data.at(i)=='&' ))
            ++offset;

    //if & is inside highlighted word
    int limit=offset+length;
    for (i=offset;i<limit;++i)
        if (KDE_ISUNLIKELY( data.at(i)=='&' ))
        {
            ++length;
            limit=qMin(data.size(),offset+length);//just safety
        }
}

void EditorTab::find()
{
    //QWidget* p=0; QWidget* next=qobject_cast<QWidget*>(parent()); while(next) { p=next; next=qobject_cast<QWidget*>(next->parent()); }
    EntryFindDialog::instance(nativeParentWidget());

    QString sel=selectionInTarget();
    if (!(sel.isEmpty()&&selectionInSource().isEmpty()))
    {
        if (sel.isEmpty())
            sel=selectionInSource();
        if (_find&&_find->options()&IGNOREACCELS)
            sel.remove('&');
            EntryFindDialog::instance()->setPattern(sel);
    }

    if ( EntryFindDialog::instance()->exec() != QDialog::Accepted )
        return;

    if (_find)
    {
        _find->resetCounts();
        _find->setPattern(EntryFindDialog::instance()->pattern());
        _find->setOptions(EntryFindDialog::instance()->options());

    }
    else // This creates a find-next-prompt dialog if needed.
    {
        _find = new KFind(EntryFindDialog::instance()->pattern(),EntryFindDialog::instance()->options(),this,EntryFindDialog::instance());
        connect(_find,SIGNAL(highlight(QString,int,int)),this, SLOT(highlightFound(QString,int,int)) );
        connect(_find,SIGNAL(findNext()),this,SLOT(findNext()));
        _find->closeFindNextDialog();
    }

    DocPosition pos;
    if (_find->options() & KFind::FromCursor)
        pos=m_currentPos;
    else if (!determineStartingPos(_find,pos))
            return;


    findNext(pos);
}

bool EditorTab::determineStartingPos(KFind* find,
                                  DocPosition& pos)
{
    if (find->options() & KFind::FindBackwards)
    {
        pos.entry=m_catalog->numberOfEntries()-1;
        pos.form=(m_catalog->isPlural(pos.entry))?
                m_catalog->numberOfPluralForms()-1:0;
    }
    else
    {
        pos.entry=0;
        pos.form=0;
    }
    return true;
}

void EditorTab::findNext(const DocPosition& startingPos)
{
    Catalog& catalog=*m_catalog;
    KFind& find=*_find;

    if (KDE_ISUNLIKELY( catalog.numberOfEntries()<=startingPos.entry ))
        return;//for the case when app wasn't able to process event before file close

    bool anotherEntry=_searchingPos.entry!=m_currentPos.entry;
    _searchingPos=startingPos;

    if (anotherEntry)
        _searchingPos.offset=0;


    QRegExp rx("[^(\\\\n)>]\n");
    QTime a;a.start();
    //_searchingPos.part=DocPosition::Source;
    bool ignoreaccels=_find->options()&IGNOREACCELS;
    bool includenotes=_find->options()&INCLUDENOTES;
    int switchOptions=DocPosition::Source|DocPosition::Target|(includenotes*DocPosition::Comment);
    int flag=1;
    while (flag)
    {

        flag=0;
        KFind::Result res = KFind::NoMatch;
        while (true)
        {
            if (find.needData()||anotherEntry||m_view->m_modifiedAfterFind)
            {
                anotherEntry=false;
                m_view->m_modifiedAfterFind=false;

                QString data;
                if (_searchingPos.part==DocPosition::Comment)
                    data=catalog.notes(_searchingPos).at(_searchingPos.form).content;
                else
                    data=catalog.catalogString(_searchingPos).string;

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
                                switchPrev(m_catalog,_searchingPos,switchOptions):
                                switchNext(m_catalog,_searchingPos,switchOptions)
                 ))
                break;
        }

        if (res==KFind::NoMatch)
        {
            //file-wide search
            if(find.shouldRestart(true,true))
            {
                flag=1;
                determineStartingPos(_find,_searchingPos);
            }
            find.resetCounts();
        }
    }
}

void EditorTab::findNext()
{
    if (_find)
    {
        findNext((m_currentPos.entry==_searchingPos.entry&&_searchingPos.part==DocPosition::Comment)?
                        _searchingPos:m_currentPos);
    }
    else
        find();

}

void EditorTab::findPrev()
{

    if (_find)
    {
        _find->setOptions(_find->options() ^ KFind::FindBackwards);
        findNext(m_currentPos);
    }
    else
    {
        find();
    }

}

void EditorTab::highlightFound(const QString &,int matchingIndex,int matchedLength)
{
    if (_find->options()&IGNOREACCELS && _searchingPos.part!=DocPosition::Comment)
    {
        QString data=m_catalog->catalogString(_searchingPos).string;
        calcOffsetWithAccels(data, matchingIndex, matchedLength);
    }

    _searchingPos.offset=matchingIndex;
    gotoEntry(_searchingPos,matchedLength);
}

void EditorTab::replace()
{
    EntryReplaceDialog::instance(nativeParentWidget());

    if (!m_view->selectionInTarget().isEmpty())
    {
        if (_replace&&_replace->options()&IGNOREACCELS)
        {
            QString tmp(m_view->selectionInTarget());
            tmp.remove('&');
            EntryReplaceDialog::instance()->setPattern(tmp);
        }
        else
            EntryReplaceDialog::instance()->setPattern(m_view->selectionInTarget());
    }


    if ( EntryReplaceDialog::instance()->exec() != QDialog::Accepted )
        return;


    if (_replace) _replace->deleteLater();// _replace=0;

    // This creates a find-next-prompt dialog if needed.
    {
        _replace = new KReplace(EntryReplaceDialog::instance()->pattern(),EntryReplaceDialog::instance()->replacement(),EntryReplaceDialog::instance()->options(),this,EntryReplaceDialog::instance());
        connect(_replace,SIGNAL(highlight(QString,int,int)),    this,SLOT(highlightFound_(QString,int,int)));
        connect(_replace,SIGNAL(findNext()),                    this,SLOT(replaceNext()));
        connect(_replace,SIGNAL(replace(QString,int,int,int)),  this,SLOT(doReplace(QString,int,int,int)));
        connect(_replace,SIGNAL(dialogClosed()),                this,SLOT(cleanupReplace()));
//         _replace->closeReplaceNextDialog();
    }
//     else
//     {
//         _replace->resetCounts();
//         _replace->setPattern(EntryReplaceDialog::instance()->pattern());
//         _replace->setOptions(EntryReplaceDialog::instance()->options());
//     }

    //m_catalog->beginMacro(i18nc("@item Undo action item","Replace"));
    m_doReplaceCalled=false;

    if (_replace->options() & KFind::FromCursor)
        replaceNext(m_currentPos);
    else
    {
        DocPosition pos;
        if (!determineStartingPos(_replace,pos)) return;
        replaceNext(pos);
    }

}


void EditorTab::replaceNext(const DocPosition& startingPos)
{
    bool anotherEntry=_replacingPos.entry!=_replacingPos.entry;
    _replacingPos=startingPos;

    if (anotherEntry)
        _replacingPos.offset=0;


    int flag=1;
    bool ignoreaccels=_replace->options()&IGNOREACCELS;
    bool includenotes=_replace->options()&INCLUDENOTES;
    kWarning()<<"includenotes"<<includenotes;
    int switchOptions=DocPosition::Target|(includenotes*DocPosition::Comment);
    while (flag)
    {
        flag=0;
        KFind::Result res=KFind::NoMatch;
        while (1)
        {
            if (_replace->needData()||anotherEntry/*||m_view->m_modifiedAfterFind*/)
            {
                anotherEntry=false;
                //m_view->m_modifiedAfterFind=false;//NOTE TEST THIS

                QString data;
                if (_replacingPos.part==DocPosition::Comment)
                    data=m_catalog->notes(_replacingPos).at(_replacingPos.form).content;
                else
                {
                    data=m_catalog->targetWithTags(_replacingPos).string;
                    if (ignoreaccels) data.remove('&');
                }
                _replace->setData(data);
            }
            res = _replace->replace();
            if (res!=KFind::NoMatch)
                break;

            if (!(
                  (_replace->options()&KFind::FindBackwards)?
                                switchPrev(m_catalog,_replacingPos,switchOptions):
                                switchNext(m_catalog,_replacingPos,switchOptions)
                 ))
                break;
        }

        if (res==KFind::NoMatch)
        {
            if((_replace->options()&KFind::FromCursor)
                &&_replace->shouldRestart(true))
            {
                flag=1;
                determineStartingPos(_replace,_replacingPos);
            }
            else
            {
                if(!(_replace->options() & KFind::FromCursor))
                     _replace->displayFinalDialog();

                _replace->closeReplaceNextDialog();
                cleanupReplace();
            }
            _replace->resetCounts();
        }
    }
}

void EditorTab::cleanupReplace()
{
    if(m_doReplaceCalled)
    {
        m_doReplaceCalled=false;
        m_catalog->endMacro();
    }
}

void EditorTab::replaceNext()
{
    replaceNext(m_currentPos);
}

void EditorTab::highlightFound_(const QString &,int matchingIndex,int matchedLength)
{
    if (_replace->options()&IGNOREACCELS)
    {
        QString data=m_catalog->targetWithTags(_replacingPos).string;
        calcOffsetWithAccels(data,matchingIndex,matchedLength);
    }

    _replacingPos.offset=matchingIndex;
    gotoEntry(_replacingPos,matchedLength);
}


void EditorTab::doReplace(const QString &newStr,int offset,int newLen,int remLen)
{
    if(!m_doReplaceCalled)
    {
        m_doReplaceCalled=true;
        m_catalog->beginMacro(i18nc("@item Undo action item","Replace"));
    }
    DocPosition pos=_replacingPos;
    if (_replacingPos.part==DocPosition::Comment)
        m_catalog->push(new SetNoteCmd(m_catalog,pos,newStr));
    else
    {
        QString oldStr=m_catalog->target(_replacingPos);

        if (_replace->options()&IGNOREACCELS)
            calcOffsetWithAccels(oldStr,offset,remLen);

        pos.offset=offset;
        m_catalog->push(new DelTextCmd(m_catalog,pos,oldStr.mid(offset,remLen)));

        if (newLen)
            m_catalog->push(new InsTextCmd(m_catalog,pos,newStr.mid(offset,newLen)));
    }
    if (pos.entry==m_currentPos.entry)
    {
        pos.offset+=newLen;
        m_view->gotoEntry(pos);
    }
}










void EditorTab::spellcheck()
{
    if (!m_sonnetDialog)
    {
        m_sonnetChecker=new Sonnet::BackgroundChecker(this);
        m_sonnetChecker->changeLanguage(enhanceLangCode(Project::instance()->langCode()));
        m_sonnetDialog=new Sonnet::Dialog(m_sonnetChecker,this);
        connect(m_sonnetDialog,SIGNAL(done(QString)),this,SLOT(spellcheckNext()));
        connect(m_sonnetDialog,SIGNAL(replace(QString,int,QString)),
            this,SLOT(spellcheckReplace(QString,int,QString)));
        connect(m_sonnetDialog,SIGNAL(stop()),this,SLOT(spellcheckStop()));
        connect(m_sonnetDialog,SIGNAL(cancel()),this,SLOT(spellcheckCancel()));

        connect(m_sonnetDialog/*m_sonnetChecker*/,SIGNAL(misspelling(QString,int)),
            this,SLOT(spellcheckShow(QString,int)));
//         disconnect(/*m_sonnetDialog*/m_sonnetChecker,SIGNAL(misspelling(QString,int)),
//             m_sonnetDialog,SLOT(slotMisspelling(QString,int)));
// 
//     connect( d->checker, SIGNAL(misspelling(const QString&, int)),
//              SLOT(slotMisspelling(const QString&, int)) );
    }

    QString text=m_catalog->msgstr(m_currentPos);
    if (!m_view->selectionInTarget().isEmpty())
        text=m_view->selectionInTarget();
    text.remove('&');
    m_sonnetDialog->setBuffer(text);

    _spellcheckPos=m_currentPos;
    _spellcheckStartPos=m_currentPos;
    _spellcheckStop=false;
    //m_catalog->beginMacro(i18n("Spellcheck"));
    _spellcheckStartUndoIndex=m_catalog->index();
    m_sonnetDialog->show();

}


void EditorTab::spellcheckNext()
{
    if (_spellcheckStop)
        return;

    do
    {
        if (!switchNext(m_catalog,_spellcheckPos))
        {
            kWarning()<<_spellcheckStartPos.entry;
            kWarning()<<_spellcheckStartPos.form;
            bool continueFromStart=
                !(_spellcheckStartPos.entry==0 && _spellcheckStartPos.form==0)
                && KMessageBox::questionYesNo(this,i18n("Lokalize has reached end of document. Do you want to continue from start?"), i18nc("@title", "Spellcheck"))==KMessageBox::Yes;
            if (continueFromStart)
            {
                _spellcheckStartPos.entry=0;
                _spellcheckStartPos.form=0;
                _spellcheckPos=_spellcheckStartPos;
            }
            else
            {
                KMessageBox::information(this,i18n("Lokalize has finished spellchecking"), i18nc("@title", "Spellcheck"));
                return;
            }
        }
    }
    while (m_catalog->msgstr(_spellcheckPos).isEmpty() || !m_catalog->isApproved(_spellcheckPos.entry));

    QString text=m_catalog->msgstr(_spellcheckPos);
    text.remove('&');
    m_sonnetDialog->setBuffer(text);
}

void EditorTab::spellcheckStop()
{
    _spellcheckStop=true;
}

void EditorTab::spellcheckCancel()
{
    m_catalog->setIndex(_spellcheckStartUndoIndex);
    gotoEntry(_spellcheckPos);
}

void EditorTab::spellcheckShow(const QString &word, int offset)
{
    QString source=m_catalog->source(_spellcheckPos);
    source.remove('&');
    if (source.contains(word))
    {
        m_sonnetDialog->setUpdatesEnabled(false);
        m_sonnetChecker->continueChecking();
        return;
    }
    m_sonnetDialog->setUpdatesEnabled(true);

    show();

    DocPosition pos=_spellcheckPos;
    int length=word.length();
    calcOffsetWithAccels(m_catalog->target(pos),offset,length);
    pos.offset=offset;

    gotoEntry(pos,length);
}

void EditorTab::spellcheckReplace(QString oldWord, int offset, const QString &newWord)
{
    DocPosition pos=_spellcheckPos;
    int length=oldWord.length();
    calcOffsetWithAccels(m_catalog->target(pos),offset,length);
    pos.offset=offset;
    if (length>oldWord.length())//replaced word contains accel mark
        oldWord=m_catalog->target(pos).mid(offset,length);

    m_catalog->push(new DelTextCmd(m_catalog,pos,oldWord));
    m_catalog->push(new InsTextCmd(m_catalog,pos,newWord));


    gotoEntry(pos,newWord.length());
}











bool EditorTab::findEntryBySourceContext(const QString& source, const QString& ctxt)
{
    DocPosition pos(0);
    do
    {
        if (m_catalog->source(pos)==source && m_catalog->context(pos.entry)==QStringList(ctxt))
        {
            gotoEntry(pos);
            return true;
        }
    }
    while (switchNext(m_catalog,pos));
    return false;
}


void EditorTab::displayWordCount()
{
    //TODO in trans and fuzzy separately
    int sourceCount=0;
    int targetCount=0;
    QRegExp rxClean(Project::instance()->markup()+'|'+Project::instance()->accel());//cleaning regexp; NOTE isEmpty()?
    QRegExp rxSplit("\\W|\\d");//splitting regexp
    DocPosition pos(0);
    do
    {
        QString msg=m_catalog->source(pos);
        msg.remove(rxClean);
        QStringList words=msg.split(rxSplit,QString::SkipEmptyParts);
        sourceCount+=words.size();

        msg=m_catalog->target(pos);
        msg.remove(rxClean);
        words=msg.split(rxSplit,QString::SkipEmptyParts);
        targetCount+=words.size();
    }
    while (switchNext(m_catalog,pos));

    KMessageBox::information(this, i18nc("@info words count",
                            "Source text words: %1<br/>Target text words: %2",
                                        sourceCount,targetCount),i18nc("@title","Word Count"));
}


