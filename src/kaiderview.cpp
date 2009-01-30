/* ****************************************************************************
  This file is part of Lokalize (some bits of KBabel code were reused)

  Copyright (C) 2007-2009 by Nick Shaforostoff <shafff@ukr.net>
  Copyright (C) 1999-2000 by Matthias Kiefer <matthias.kiefer@gmx.de>
                2001-2004 by Stanislav Visnovsky <visnovsky@kde.org>

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

#include "kaiderview.h"
#include "project.h"
#include "catalog.h"
#include "cmd.h"
#include "prefs_lokalize.h"
#include "syntaxhighlighter.h"

#include <QTimer>
#include <QMenu>
#include <QDragEnterEvent>

#include <QLabel>
#include <QHBoxLayout>


#ifdef XLIFF
#include <QPixmap>
#include <QPushButton>
#include <QPainter>
#include <QStyle>
#include <QApplication>
#include <QStyleOptionButton>
#endif

#include <ktabbar.h>
#include <kled.h>
#include <kmessagebox.h>
#include <klocale.h>
#include <kdebug.h>
#include <kurl.h>
#include <kstandardshortcut.h>
#include <kcolorscheme.h>


//parent is set on qsplitter insertion
class LedsWidget:public QWidget
{
public:
    LedsWidget(QWidget* parent);
private:
    void contextMenuEvent(QContextMenuEvent* event);
public:
    KLed* ledFuzzy;
    KLed* ledUntr;
    //KLed* ledErr;
    QLabel* lblColumn;
};
LedsWidget::LedsWidget(QWidget* parent): QWidget(parent)
{
    KColorScheme colorScheme(QPalette::Normal);

    QHBoxLayout* layout=new QHBoxLayout(this);
    layout->addStretch();
    layout->addWidget(new QLabel(i18nc("@label whether entry is fuzzy","Fuzzy:"),this));
    layout->addWidget(ledFuzzy=new KLed(colorScheme.foreground(KColorScheme::NeutralText)/*Qt::green*/,KLed::Off,KLed::Sunken,KLed::Rectangular));
    layout->addWidget(new QLabel(i18nc("@label whether entry is untranslated","Untranslated:"),this));
    layout->addWidget(ledUntr=new KLed(colorScheme.foreground(KColorScheme::NegativeText)/*Qt::red*/,KLed::Off,KLed::Sunken,KLed::Rectangular));
    layout->addSpacing(1);
    layout->addWidget(lblColumn=new QLabel(this));
    layout->addStretch();
    setMaximumHeight(minimumSizeHint().height());
}
void LedsWidget::contextMenuEvent(QContextMenuEvent* event)
{
    QMenu menu;
    menu.addAction(i18nc("@action","Hide"));
    if (!menu.exec(event->globalPos()))
        return; //NOTE the config doesn't seem to work
    Settings::setLeds(false); 
    Settings::self()->writeConfig();
    hide();
}



void ProperTextEdit::keyPressEvent(QKeyEvent *keyEvent)
{
    // ALT+123 feature TODO this is general so should be on another level
    if( (keyEvent->modifiers()&Qt::AltModifier)
         &&!keyEvent->text().isEmpty()
         &&keyEvent->text().at(0).isDigit() )
    {
        QString text=keyEvent->text();
        while (!text.isEmpty()&& text.at(0).isDigit() )
        {
            m_currentUnicodeNumber = 10*m_currentUnicodeNumber+(text.at(0).digitValue());
            text.remove(0,1);
        }
    }
    else
        KTextEdit::keyPressEvent(keyEvent);
}

void ProperTextEdit::keyReleaseEvent(QKeyEvent* e)
{
    if ( (e->key()==Qt::Key_Alt) && m_currentUnicodeNumber >= 32 )
    {
        insertPlainText(QChar( m_currentUnicodeNumber ));
        m_currentUnicodeNumber=0;
    }
    else
        KTextEdit::keyReleaseEvent(e);
}

QString ProperTextEdit::toPlainText()
{
    QTextCursor cursor = textCursor();
    cursor.select(QTextCursor::Document);
    QString text=cursor.selectedText();
    text.replace(QChar(8233),'\n');
/*
    int ii=text.size();
    while(--ii>=0)
        kWarning()<<text.at(ii).unicode();
*/
    return text;
}


#ifdef XLIFF
//BEGIN XLIFF


inline static QImage generateImage(QString str, ProperTextEdit* w)
{
    str.prepend(' ');
    QFontMetrics metrics(w->currentFont());
    QRect rect=metrics.boundingRect(str).adjusted(0,0,5,0);
    rect.moveTo(0,0);

    QImage result(rect.size(),QImage::Format_ARGB32);
    result.fill(0);//0xAARRGGBB
    QPainter painter(&result);
    QStyleOptionButton opt;
    opt.text=str;
    opt.rect=rect;
    QApplication::style()->drawControl(QStyle::CE_PushButton,&opt,&painter);

    return result;
}

void ProperTextEdit::setContent(const CatalogString& catStr, const CatalogString& refStr)
{
    //kWarning()<<"";
    //kWarning()<<"START";
    //kWarning()<<str<<ranges.size();
    //prevent undo tracking system from recording this 'action'
    document()->blockSignals(true);
    clear();
    QTextCursor cur=textCursor();


    QMap<int,int> posToTagRange;
    int i=catStr.ranges.size();
    //if (i) kWarning()<<"tags we got:";
    while(--i>=0)
    {
        //kWarning()<<"\t"<<catStr.ranges.at(i).getElementName()<<catStr.ranges.at(i).id<<catStr.ranges.at(i).start<<catStr.ranges.at(i).end;
        posToTagRange.insert(catStr.ranges.at(i).start,i);
        posToTagRange.insert(catStr.ranges.at(i).end,i);
    }

    QMap<QString,int> sourceTagIdToIndex=refStr.tagIdToIndex();

    i=0;
    int prev=0;
    while ((i = catStr.string.indexOf(TAGRANGE_IMAGE_SYMBOL, i)) != -1)
    {
        //kWarning()<<"HAPPENED!!";
        int tagRangeIndex=posToTagRange.value(i);
        cur.insertText(catStr.string.mid(prev,i-prev));

        TagRange ourRange=catStr.ranges.at(tagRangeIndex);
        QString name=' '+ourRange.id;
        QString text;
        if (sourceTagIdToIndex.isEmpty())
            text=QString::number(tagRangeIndex);
        else
            text=QString::number(sourceTagIdToIndex.value(ourRange.id));
        if (ourRange.start!=ourRange.end)
        {
            //kWarning()<<"b"<<i;
            if (ourRange.start==i)
            {
                //kWarning()<<"\t\tstart:"<<ourRange.getElementName()<<ourRange.id<<ourRange.start;
                text.append(" {");
                name.append("-start");
            }
            else
            {
                //kWarning()<<"\t\tend:"<<ourRange.getElementName()<<ourRange.id<<ourRange.end;
                text.prepend("} ");
                name.append("-end");
            }
        }
        document()->addResource(QTextDocument::ImageResource, QUrl(name), generateImage(text,this));
        cur.insertImage(name);//NOTE what if twice the same name?

        prev=++i;
    }
    cur.insertText(catStr.string.mid(prev));

    document()->blockSignals(false);
}

//END XLIFF
#endif




















KAiderView::KAiderView(QWidget *parent,Catalog* catalog/*,keyEventHandler* kh*/)
    : QSplitter(Qt::Vertical,parent)
    , _catalog(catalog)
    , _msgidEdit(new ProperTextEdit(this))
    , _msgstrEdit(new ProperTextEdit(this))
    , m_msgidHighlighter(new SyntaxHighlighter(_msgidEdit->document()))
    , m_msgstrHighlighter(new SyntaxHighlighter(_msgstrEdit->document()))
    , m_pluralTabBar(new KTabBar(this))
    , _leds(0)
    , _currentEntry(-1)
    , m_approvementState(true)
    , m_modifiedAfterFind(false)
{
    m_pluralTabBar->hide();
    _msgidEdit->setWhatsThis(i18n("<qt><p><b>Original String</b></p>\n"
                                  "<p>This part of the window shows the original message\n"
                                  "of the currently displayed entry.</p></qt>"));

    _msgidEdit->setReadOnly(true);

    //apply changes to catalog via undo/redo
    connect (_msgstrEdit->document(), SIGNAL(contentsChange(int,int,int)), this, SLOT(contentsChanged(int,int,int)));
    connect (_msgstrEdit, SIGNAL(cursorPositionChanged()), this, SLOT(cursorPositionChanged()));


    _msgidEdit->setUndoRedoEnabled(false);
    _msgstrEdit->setUndoRedoEnabled(false);
    _msgstrEdit->setAcceptRichText(false);
    _msgstrEdit->installEventFilter(this);
    _msgidEdit->installEventFilter(this);

    addWidget(m_pluralTabBar);
    addWidget(_msgidEdit);
    addWidget(_msgstrEdit);

    QWidget::setTabOrder(_msgstrEdit,_msgidEdit);
    QWidget::setTabOrder(_msgidEdit,_msgstrEdit);
    setFocusProxy(_msgstrEdit);
//     QTimer::singleShot(3000,this,SLOT(setupWhatsThis()));
    settingsChanged();
}

KAiderView::~KAiderView()
{
    /*
    delete _msgidEdit;
    delete _msgstrEdit;
    delete m_pluralTabBar;
    */
}

// void KAiderView::setupWhatsThis()
// {
//     _msgidEdit->setWhatsThis(i18n("<qt><p><b>Original String</b></p>\n"
//                                   "<p>This part of the window shows the original message\n"
//                                   "of the currently displayed entry.</p></qt>"));
// }

static bool isMasked(const QString& str, uint col)
{
    if(col == 0 || str.isEmpty())
        return false;

    uint counter=0;
    int pos=col;

    while(pos >= 0 && str.at(pos) == '\\')
    {
        counter++;
        pos--;
    }

    return !(bool)(counter%2);
}


bool KAiderView::eventFilter(QObject* obj, QEvent* event)
{
    if (event->type() != QEvent::KeyPress)
        return false;//QObject::eventFilter(obj, event);
    QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);



    if(keyEvent->matches(QKeySequence::MoveToPreviousPage))
    {
        emit signalGotoPrev();
        return true;
    }
    else if(keyEvent->matches(QKeySequence::MoveToNextPage))
    {
        emit signalGotoNext();
        return true;
    }
    if (obj!=_msgstrEdit)
        return false;


    static QString spclChars("abfnrtv'\"?\\");
    if(keyEvent->matches(QKeySequence::Undo))
    {
        if (_catalog->canUndo())
        {
            emit signalUndo();
            approvedEntryDisplayed(_catalog->isApproved(_currentEntry));
            return true;
        }
    }
    else if(keyEvent->matches(QKeySequence::Redo))
    {
        if (_catalog->canRedo())
        {
            emit signalRedo();
            approvedEntryDisplayed(_catalog->isApproved(_currentEntry));
            return true;
        }
    }
    else if (keyEvent->modifiers() == (Qt::AltModifier|Qt::ControlModifier))
    {
        if(keyEvent->key()==Qt::Key_Home)
        {
            emit signalGotoFirst();
            return true;
        }
        else if(keyEvent->key()==Qt::Key_End)
        {
            emit signalGotoLast();
            return true;
        }
    }
    else if (!keyEvent->modifiers()&&(keyEvent->key()==Qt::Key_Backspace||keyEvent->key()==Qt::Key_Delete))
    {
        //only for cases when:
        //-BkSpace was hit and cursor was atStart
        //-Del was hit and cursor was atEnd
        if (KDE_ISUNLIKELY( !_catalog->isApproved(_currentEntry) && !_msgstrEdit->textCursor().hasSelection() ))
                          //&& (_msgstrEdit->textCursor().atStart()||_msgstrEdit->textCursor().atEnd()) ))
            toggleApprovement(true);
        return false;
    }
    //clever editing
    else if(keyEvent->key()==Qt::Key_Return||keyEvent->key()==Qt::Key_Enter)
    {
        QString str=_msgstrEdit->toPlainText();
        QTextCursor t=_msgstrEdit->textCursor();
        int pos=t.position();
        QString ins;
        if( keyEvent->modifiers()&Qt::ShiftModifier )
        {
            if(pos>0
               &&!str.isEmpty()
               &&str.at(pos-1)=='\\'
               &&!isMasked(str,pos-1))
            {
                ins='n';
            }
            else
            {
                ins="\\n";
            }
        }
        else if(!(keyEvent->modifiers()&Qt::ControlModifier))
        {
            if(pos>0
               &&!str.isEmpty()
               &&!str.at(pos-1).isSpace())
            {
                if(str.at(pos-1)=='\\'
                   &&!isMasked(str,pos-1))
                    ins='\\';
                // if there is no new line at the end
                if(pos<2||str.midRef(pos-2,2)!="\\n")
                    ins+=' ';
            }
            else if(str.isEmpty())
            {
                ins="\\n";
            }
        }
        if (!str.isEmpty())
            ins+='\n';
        _msgstrEdit->insertPlainText(ins);

        return true;
    }
    else if(keyEvent->key() == Qt::Key_Tab)
    {
        _msgstrEdit->insertPlainText("\\t");
        return true;
    }
    else if( (keyEvent->modifiers()&Qt::ControlModifier)?
                (keyEvent->key()==Qt::Key_D) :
                (keyEvent->key()==Qt::Key_Delete))
    {
        QTextCursor t=_msgstrEdit->textCursor();
        if(!t.hasSelection())
        {
            int pos=t.position();
            QString str=_msgstrEdit->toPlainText();
            //workaround for Qt/X11 bug: if Del on NumPad is pressed, then pos is beyond end
            if (pos==str.size()) --pos;
            if(!str.isEmpty()
                &&str.at(pos) == '\\'
                &&!isMasked(str,pos))
            {
                if(pos<str.length()-1&&spclChars.contains(str.at(pos+1)))
                    t.deleteChar();
            }
        }

        t.deleteChar();
        _msgstrEdit->setTextCursor(t);

        return true;
    }
    else if( (!keyEvent->modifiers()&&keyEvent->key()==Qt::Key_Backspace)
            || ( ( keyEvent->modifiers() & Qt::ControlModifier ) && keyEvent->key() == Qt::Key_H ) )
    {
        QTextCursor t=_msgstrEdit->textCursor();
        if(!t.hasSelection())
        {
            int pos=t.position();
            QString str=_msgstrEdit->toPlainText();
            if(!str.isEmpty() && pos>0 && spclChars.contains(str.at(pos-1)))
            {
                if(pos>1 && str.at(pos-2)=='\\' && !isMasked(str,pos-2))
                    t.deletePreviousChar();
            }

        }
        t.deletePreviousChar();
        _msgstrEdit->setTextCursor(t);

        return true;
    }
    else if(keyEvent->text()=="\"")
    {
        QTextCursor t=_msgstrEdit->textCursor();
        int pos=t.position();
        QString str=_msgstrEdit->toPlainText();
        QString ins;

        if(pos==0 || str.at(pos-1)!='\\' || isMasked(str,pos-1))
            ins="\\\"";
        else
            ins="\"";

        t.insertText(ins);
        _msgstrEdit->setTextCursor(t);
        return true;
    }
    else if( keyEvent->key() == Qt::Key_Space && ( keyEvent->modifiers() & Qt::AltModifier ) )
    {
        _msgstrEdit->insertPlainText(QChar(0x00a0U));
        return true;
    }

    return false;
}




void KAiderView::settingsChanged()
{
    //Settings::self()->config()->setGroup("Editor");
    _msgidEdit->document()->setDefaultFont(Settings::msgFont());
    _msgstrEdit->document()->setDefaultFont(Settings::msgFont());
    if (Settings::leds())
    {
        if (_leds)
            _leds->show();
        else
        {
            _leds=new LedsWidget(this);
            if (!_catalog->isApproved(_currentEntry))
                _leds->ledFuzzy->on();
            if (_catalog->msgstr(_currentPos).isEmpty())
                _leds->ledUntr->on();
            cursorPositionChanged();
            insertWidget(2,_leds);
        }
    }
    else if (_leds)
        _leds->hide();

}

void KAiderView::cursorPositionChanged()
{
    if (_leds)
        _leds->lblColumn->setText(i18nc("@info:label cursor position", "Column: %1", _msgstrEdit->textCursor().columnNumber()));
}

void KAiderView::contentsChanged(int offset, int charsRemoved, int charsAdded)
{
    //kWarning()<<"called";
    //kWarning()<<"!!!!!!!!!!!! offset"<<offset<<"charsRemoved"<<charsRemoved<<"_oldMsgstr"<<_oldMsgstr;
    QString editText=_msgstrEdit->toPlainText();
    if (KDE_ISUNLIKELY( _currentEntry==-1 || editText==_oldMsgstr ))
        return;

    DocPosition pos=_currentPos;
    pos.offset=offset;
    //kWarning()<<"offset"<<offset<<"charsRemoved"<<charsRemoved<<"_oldMsgstr"<<_oldMsgstr;

#ifdef XLIFF

    QString target=_catalog->targetWithTags(pos).string;
//BEGIN XLIFF markup handling
    //protect from tag removal
    bool markupRemoved=charsRemoved && target.mid(offset,charsRemoved).contains(TAGRANGE_IMAGE_SYMBOL);
    bool markupAdded=charsAdded && editText.mid(offset,charsAdded).contains(TAGRANGE_IMAGE_SYMBOL);
    if (markupRemoved || markupAdded)
    {
        bool modified=false;
        CatalogString targetWithTags=_catalog->targetWithTags(_currentPos);
        //special case when the user presses Del
        if (!charsAdded && charsRemoved==1)
        {
            int i=targetWithTags.ranges.size();
            while(--i>=0)
            {
                if (targetWithTags.ranges.at(i).start==offset || targetWithTags.ranges.at(i).end==offset)
                {
                    modified=true;
                    pos.offset=targetWithTags.ranges.at(i).start;
                    _catalog->push(new DelTagCmd(_catalog,pos));
                }
            }
        }
        else if (!markupAdded) //check if all { plus } tags were selected
        {
            int lenDecrement=0;
            QMap<int,int> tagPlaces;//>0 if both start and end parts of tag were deleted
                                    //1 means this is start, 2 means this is end
            int t=offset;
            while ((t=target.indexOf(TAGRANGE_IMAGE_SYMBOL,t))!=-1 && t<(charsRemoved+offset))
                tagPlaces[t++]=false;
            
            int i=targetWithTags.ranges.size();
            while(--i>=0)
            {
                qWarning()<<targetWithTags.ranges.at(i).getElementName();
                if (tagPlaces.contains(targetWithTags.ranges.at(i).start)
                    &&tagPlaces.contains(targetWithTags.ranges.at(i).end))
                {
                    qWarning()<<"start"<<targetWithTags.ranges.at(i).start<<"end"<<targetWithTags.ranges.at(i).end;
                    tagPlaces[targetWithTags.ranges.at(i).end]=2;
                    tagPlaces[targetWithTags.ranges.at(i).start]=1;
                }
            }

            QMap<int,int>::const_iterator it = tagPlaces.constBegin();
            while (it != tagPlaces.constEnd())
            {
                qWarning()<<it.key()<<it.value();
                if (!it.value())
                    break;
                ++it;
            }
            if (it==tagPlaces.constEnd())//all indexes are ok.
            {
                modified=true;
                kWarning()<<"all indexes are ok";
                it = tagPlaces.constBegin();
                while (it != tagPlaces.constEnd())
                {
                    if (it.value()==1)
                    {
                        pos.offset=it.key();
                        DelTagCmd* cmd=new DelTagCmd(_catalog,pos);
                        _catalog->push(cmd);
                        //lenDecrement+=1+cmd->tag().isPaired();
                        qWarning()<<"lenDecrement"<<lenDecrement;
                    }
                    ++it;
                }
                //charsRemoved-=lenDecrement;
                //qWarning()<<"charsRemoved"<<charsRemoved<<"offset"<<offset;
                pos.offset=offset;
                if (charsRemoved)
                {
                    QString rText=target.mid(offset,charsRemoved);
                    rText.remove(TAGRANGE_IMAGE_SYMBOL);
                    qWarning()<<"rText"<<rText<<"offset"<<offset;
                    _catalog->push(new DelTextCmd(_catalog,pos,rText));
                }
                if (charsAdded)
                    _catalog->push(new InsTextCmd(_catalog,pos,editText.mid(offset,charsAdded)));

            }
        }
            
            /*
    CatalogString sourceWithTags=_catalog->sourceWithTags(_currentPos);
    int count=sourceWithTags.ranges.size();
    if (count)
    {
        QMap<QString,int> tagIdToIndex=_catalog->targetWithTags(_currentPos).tagIdToIndex();
        bool hasActive=false;
      
        
        
        TagRange tag=sourceWithTags.ranges.at(txt->data().toInt());
        QTextCursor cursor=_msgstrEdit->textCursor();
        tag.start=qMin(cursor.anchor(),cursor.position());
        tag.end=qMax(cursor.anchor(),cursor.position());
        _catalog->push(new InsTagCmd(_catalog,_currentPos,tag));

        */

        refreshMsgEdit(/*keepCursor*/true,_catalog->sourceWithTags(pos));
        if (!modified)
            return;
    }
//END XLIFF markup handling
#endif
    else
    {

        if (charsRemoved)
            _catalog->push(new DelTextCmd(_catalog,pos,_oldMsgstr.mid(offset,charsRemoved)));

        _oldMsgstr=editText;//newStr becomes OldStr
        //kWarning()<<"char"<<editText[offset].unicode();
        if (charsAdded)
            _catalog->push(new InsTextCmd(_catalog,pos,editText.mid(offset,charsAdded)));
    }

    m_modifiedAfterFind=true;

    if (_leds)
    {
        if (_catalog->msgstr(pos).isEmpty())
            _leds->ledUntr->on();
        else
            _leds->ledUntr->off();
    }

    if (!_catalog->isApproved(_currentEntry)&&Settings::autoApprove())
        toggleApprovement(true);

    // for mergecatalog (remove entry from index)
    // and for statusbar
    emit signalChanged(pos.entry);
}

void KAiderView::approvedEntryDisplayed(bool approved)
{
    //kWarning()<<"approvedEntryDisplayed. entry:"<<_currentEntry<<"approved:"<<approved;
    if (KDE_ISUNLIKELY( _currentEntry==-1 ))
    {
        //kWarning()<<"approvedEntryDisplayed returning";
        return;
    }
    m_approvementState=approved;

    m_msgstrHighlighter->setApprovementState(approved);
    m_msgstrHighlighter->rehighlight();

    if (approved)
    {
        _msgstrEdit->viewport()->setBackgroundRole(QPalette::Base);
        if (_leds)
            _leds->ledFuzzy->off();
    }
    else
    {
        _msgstrEdit->viewport()->setBackgroundRole(QPalette::AlternateBase);
        if (_leds)
            _leds->ledFuzzy->on();
    }
}


/**
 * makes MsgEdit reflect current entry
 **/
CatalogString KAiderView::refreshMsgEdit(bool keepCursor, const CatalogString& refStr)
{
    //kWarning()<<"called";
    ProperTextEdit& msgEdit=*_msgstrEdit;
    QTextCursor cursor=msgEdit.textCursor();

    int pos=cursor.position();
    int anchor=cursor.anchor();

#ifdef XLIFF
    CatalogString targetWithTags=_catalog->targetWithTags(_currentPos);
    QString target=targetWithTags.string;
#else
    QString target=_catalog->target(_currentPos);
#endif
    _oldMsgstr=target;

    if (!keepCursor && msgEdit.toPlainText()!=target)
    {
        pos=0;
        anchor=0;
    }


    disconnect (msgEdit.document(), SIGNAL(contentsChange(int,int,int)), this, SLOT(contentsChanged(int,int,int)));

#ifdef XLIFF
    msgEdit.setContent(targetWithTags,refStr);
#else
    //prevent undo tracking system from recording this 'action'
    msgEdit.document()->blockSignals(true);
    msgEdit.setPlainText(target);
    msgEdit.document()->blockSignals(false);
#endif

    QTextCursor t=msgEdit.textCursor();
    t.movePosition(QTextCursor::Start);
    if (pos || anchor)
    {
        //int anchorDiff=pos-anchor;
/*        t.movePosition(QTextCursor::NextCharacter,QTextCursor::MoveAnchor,qMin(anchor,pos));
        t.movePosition(QTextCursor::NextCharacter,QTextCursor::KeepAnchor,qMax(anchor,pos));
        */
        // I don't know why the following (more correct) code does not work
        t.movePosition(QTextCursor::NextCharacter,QTextCursor::MoveAnchor,anchor);
        int length=pos-anchor;
        if (length)
            t.movePosition(length<0?QTextCursor::PreviousCharacter:QTextCursor::NextCharacter,QTextCursor::KeepAnchor,qAbs(length));
    }
    msgEdit.setTextCursor(t);


    approvedEntryDisplayed(_catalog->isApproved(_currentPos.entry));
    //m_msgstrHighlighter->rehighlight(); done in approvedEntryDisplayed()
    connect (msgEdit.document(), SIGNAL(contentsChange(int,int,int)), this, SLOT(contentsChanged(int,int,int)));
    //kWarning()<<"finished";
    return targetWithTags; //for the sake of not calling XliffStorage/doContent twice
}


//main function in this file :)
void KAiderView::gotoEntry(const DocPosition& pos,int selection)
{
    setUpdatesEnabled(false);

    _currentPos=pos;
    _currentEntry=pos.entry;

    if (KDE_ISUNLIKELY( _catalog->isPlural(_currentEntry)))
    {
        if (KDE_ISUNLIKELY( _catalog->numberOfPluralForms()!=m_pluralTabBar->count() ))
        {
            int i=m_pluralTabBar->count();
            if (_catalog->numberOfPluralForms()>m_pluralTabBar->count())
                while (i<_catalog->numberOfPluralForms())
                    m_pluralTabBar->addTab(i18nc("@title:tab","Plural Form %1",++i));
            else
                while (i>_catalog->numberOfPluralForms())
                    m_pluralTabBar->removeTab(i--);
        }
        m_pluralTabBar->show();
        m_pluralTabBar->blockSignals(true);
        m_pluralTabBar->setCurrentIndex(_currentPos.form);
        m_pluralTabBar->blockSignals(false);
    }
    else
        m_pluralTabBar->hide();

#ifdef XLIFF
    CatalogString sourceWithTags=_catalog->sourceWithTags(_currentPos);
    _msgidEdit->setContent(sourceWithTags);
    m_msgidHighlighter->rehighlight(); //explicitly because setContent disables signals
#else
    _msgidEdit->setPlainText(_catalog->msgid(_currentPos)/*, _catalog->msgctxt(_currentIndex)*/);
#endif


#ifdef UNWRAP_MSGID
    unwrap(_msgidEdit);
#endif

    //kWarning()<<"calling refreshMsgEdit";
    //refreshMsgEdit() sets msgstredit text along the way
    QString targetString=refreshMsgEdit(false,sourceWithTags).string;
    bool untrans=targetString.isEmpty();
    if (_leds)
    {
        if (untrans)
            _leds->ledUntr->on();
        else
            _leds->ledUntr->off();
    }
    ProperTextEdit* msgEdit=_msgstrEdit;
    QTextCursor t=msgEdit->textCursor();
    t.movePosition(QTextCursor::Start);

    if (pos.offset || selection)
    {
        if (pos.part==Msgid)
        {
            msgEdit=_msgidEdit;
            t=msgEdit->textCursor();
            t.movePosition(QTextCursor::Start);
        }

        t.movePosition(QTextCursor::NextCharacter,QTextCursor::MoveAnchor,pos.offset);
        //NOTE this was kinda bug due to on-the-fly msgid wordwrap
        if (selection)
            t.movePosition(QTextCursor::NextCharacter,QTextCursor::KeepAnchor,selection);
    }
    else if (!untrans)
    {
        //what if msg starts with a tag?
        if (KDE_ISUNLIKELY( targetString.startsWith('<') ))
        {
            int offset=targetString.indexOf(QRegExp(">[^<]"));
            if ( offset!=-1 )
                t.movePosition(QTextCursor::NextCharacter,QTextCursor::MoveAnchor,offset+1);
        }
        else if (KDE_ISUNLIKELY( _catalog->msgstr(_currentPos).startsWith(TAGRANGE_IMAGE_SYMBOL) ))
        {
            int offset=targetString.indexOf(QRegExp("[^"+TAGRANGE_IMAGE_SYMBOL+']'));
            if ( offset!=-1 )
                t.movePosition(QTextCursor::NextCharacter,QTextCursor::MoveAnchor,offset+1);
        }
    }

    msgEdit->setTextCursor(t);

    //for undo/redo tracking
    //_oldMsgstr=_catalog->msgstr(_currentPos); :::this is done in refreshMsgEdit()

    //prevent undo tracking system from recording this 'action'
//     disconnect (_msgstrEdit->document(), SIGNAL(contentsChange(int,int,int)), this, SLOT(contentsChanged(int,int,int)));
//     m_msgstrHighlighter->rehighlight();
//     connect (_msgstrEdit->document(), SIGNAL(contentsChange(int,int,int)), this, SLOT(contentsChanged(int,int,int)));

    _msgstrEdit->setFocus();

    //kWarning()<<"anchor"<<t.anchor()<<"pos"<<t.position();
    setUpdatesEnabled(true);
}
/*
void KAiderView::dragEnterEvent(QDragEnterEvent* event)
{
    if(event->mimeData()->hasUrls() && event->mimeData()->urls().first().path().endsWith(".po"))
        event->acceptProposedAction();
}

void KAiderView::dropEvent(QDropEvent *event)
{
    emit fileOpenRequested(KUrl(event->mimeData()->urls().first()));
    event->acceptProposedAction();
}



*/





//BEGIN edit actions that are easier to do in this class
void KAiderView::clearMsgStr()
{
    if (KDE_ISUNLIKELY( _currentEntry==-1 ))
        return;

    _currentPos.offset=0;
    _catalog->push(new DelTextCmd(_catalog,_currentPos,_catalog->msgstr(_currentPos)));
    if (!_catalog->isApproved(_currentEntry))
        toggleApprovement(true);

    gotoEntry(_currentPos);
    emit signalChanged(_currentEntry);
}

void KAiderView::toggleBookmark(bool checked)
{
    if (KDE_ISUNLIKELY( _currentEntry==-1 ))
        return;

    _catalog->setBookmark(_currentEntry,checked);
}

void KAiderView::toggleApprovement(bool approved)
{
    //kWarning()<<"called";
    if (KDE_ISUNLIKELY( _currentEntry==-1 ))
        return;

    _catalog->push(new ToggleApprovementCmd(_catalog,_currentEntry,approved));
    approvedEntryDisplayed(approved);
}


void KAiderView::msgid2msgstr()
{
    QString text(_catalog->msgid(_currentPos));
    QString out;
    QString ctxt(_catalog->msgctxt(_currentPos.entry));

    //TODO ask for the fillment if the first time.
    // this is KDE specific:
    if( ctxt.startsWith( "NAME OF TRANSLATORS" ) || text.startsWith( "_: NAME OF TRANSLATORS\\n" ))
    {
        if (!_catalog->msgstr(_currentPos).isEmpty())
            out=", ";
        out+=Settings::authorLocalizedName();
    }
    else if( ctxt.startsWith( "EMAIL OF TRANSLATORS" ) || text.startsWith( "_: EMAIL OF TRANSLATORS\\n" ))
    {
        if (!_catalog->msgstr(_currentPos).isEmpty())
            out=", ";
        out+=Settings::authorEmail();
    }
    else if( /*_catalog->isGeneratedFromDocbook() &&*/ text.startsWith( "ROLES_OF_TRANSLATORS" ) )
    {
        if (!_catalog->msgstr(_currentPos).isEmpty())
            out='\n';
        out+="<othercredit role=\\\"translator\\\">\n"
        "<firstname></firstname><surname></surname>\n"
        "<affiliation><address><email>"+Settings::authorEmail()+"</email></address>\n"
        "</affiliation><contrib></contrib></othercredit>";
    }
    else if( text.startsWith( "CREDIT_FOR_TRANSLATORS" ) )
    {
        if (!_catalog->msgstr(_currentPos).isEmpty())
            out='\n';
        out+="<para>"+Settings::authorLocalizedName()+'\n'+
            "<email>"+Settings::authorEmail()+"</email></para>";
    }
/*    else if(text.contains(_catalog->miscSettings().singularPlural))
    {
        text.replace(_catalog->miscSettings().singularPlural,"");
    }*/
    // end of KDE specific part


    if (out.isEmpty())
    {
        DocPosition pos=_currentPos;pos.offset=0;
        //_msgstrEdit->setPlainText(text);
        _catalog->push(new DelTextCmd(_catalog,pos,_catalog->target(_currentPos)));
        _oldMsgstr="";//newStr becomes OldStr
        _catalog->push(new InsTextCmd(_catalog,pos,text));
        refreshMsgEdit(/*keepCursor*/false,_catalog->sourceWithTags(pos));
        if (KDE_ISUNLIKELY( !_catalog->isApproved(pos.entry) ))
            toggleApprovement(true);
    }
    else
    {
        QTextCursor t=_msgstrEdit->textCursor();
        t.movePosition(QTextCursor::End);
        t.insertText(out);
        _msgstrEdit->setTextCursor(t);
    }
}



void KAiderView::unwrap(ProperTextEdit* editor)
{
    if (!editor)
        editor=_msgstrEdit;

    QTextCursor t=editor->document()->find(QRegExp("[^(\\\\n)]$"));
    if (t.isNull())
        return;

    if (editor==_msgstrEdit)
        _catalog->beginMacro(i18nc("@item Undo action item","Unwrap"));
    t.movePosition(QTextCursor::EndOfLine);
    if (!t.atEnd())
        t.deleteChar();

    QRegExp rx("[^(\\\\n)>]$");
    //remove '\n's skipping "\\\\n"
    while (!(t=editor->document()->find(rx,t)).isNull())
    {
        t.movePosition(QTextCursor::EndOfLine);
        if (!t.atEnd())
            t.deleteChar();
    }
    if (editor==_msgstrEdit)
        _catalog->endMacro();
}

void KAiderView::insertTerm(const QString& term)
{
    _msgstrEdit->insertPlainText(term);
    _msgstrEdit->setFocus();
}

void KAiderView::tagMenu()
{
    QMenu menu;
    QAction* txt=0;

    CatalogString sourceWithTags=_catalog->sourceWithTags(_currentPos);
    int count=sourceWithTags.ranges.size();
    if (count)
    {
        QMap<QString,int> tagIdToIndex=_catalog->targetWithTags(_currentPos).tagIdToIndex();
        bool hasActive=false;
        for (int i=0;i<count;++i)
        {
            //txt=menu.addAction(sourceWithTags.ranges.at(i));
            txt=menu.addAction(QString::number(i)+" "+sourceWithTags.ranges.at(i).id);
            txt->setData(QVariant(i));
            if (!hasActive && !tagIdToIndex.contains(sourceWithTags.ranges.at(i).id))
            {
                hasActive=true;
                menu.setActiveAction(txt);
            }
        }
        txt=menu.exec(_msgstrEdit->mapToGlobal(_msgstrEdit->cursorRect().bottomRight()));
        if (!txt)
            return;
        TagRange tag=sourceWithTags.ranges.at(txt->data().toInt());
        QTextCursor cursor=_msgstrEdit->textCursor();
        tag.start=qMin(cursor.anchor(),cursor.position());
        tag.end=qMax(cursor.anchor(),cursor.position());
        _catalog->push(new InsTagCmd(_catalog,_currentPos,tag));
        refreshMsgEdit(/*keepCursor*/false,_catalog->sourceWithTags(_currentPos));
        cursor.movePosition(QTextCursor::NextCharacter,QTextCursor::MoveAnchor,tag.end+1);
    }
    else
    {
        if (KDE_ISUNLIKELY( Project::instance()->markup().isEmpty() ))
            return;

        //QRegExp tag("(<[^>]*>)+|\\&\\w+\\;");
        QRegExp tag(Project::instance()->markup());
        tag.setMinimal(true);
        QString en(_msgidEdit->toPlainText());
        QString target(_msgstrEdit->toPlainText());
        en.remove('\n');
        target.remove('\n');
        int pos=0;
        //tag.indexIn(en);
        int posInMsgStr=0;
        while ((pos=tag.indexIn(en,pos))!=-1)
        {
    /*        QString str(tag.cap(0));
            str.replace("&","&&");*/
            txt=menu.addAction(tag.cap(0));
            pos+=tag.matchedLength();

            if (posInMsgStr!=-1 && (posInMsgStr=target.indexOf(tag.cap(0),posInMsgStr))==-1)
                menu.setActiveAction(txt);
            else if (posInMsgStr!=-1)
                posInMsgStr+=tag.matchedLength();
        }
        if (!txt)
            return;

        //txt=menu.exec(_msgidEdit->mapToGlobal(QPoint(0,0)));
        txt=menu.exec(_msgstrEdit->mapToGlobal(_msgstrEdit->cursorRect().bottomRight()));
        if (txt)
            _msgstrEdit->insertPlainText(txt->text());

    }
}

//END edit actions that are easier to do in this class




#include "kaiderview.moc"
