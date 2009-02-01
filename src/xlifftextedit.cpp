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

#include "xlifftextedit.h"
#include "catalog.h"
#include "cmd.h"
#include "syntaxhighlighter.h"
#include "prefs_lokalize.h"
#include "project.h"

#include <QPixmap>
#include <QPushButton>
#include <QPainter>
#include <QStyle>
#include <QApplication>
#include <QStyleOptionButton>
#include <QMimeData>
#include <QMetaType>

#include <QMenu>



inline static QImage generateImage(QString str, XliffTextEdit* w)
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



XliffTextEdit::XliffTextEdit(Catalog* catalog, DocPosition::Part part, QWidget* parent)
    : KTextEdit(parent)
    , m_currentUnicodeNumber(0)
    , m_catalog(catalog)
    , m_part(part)
    , m_highlighter(new SyntaxHighlighter(document()))
{
    setReadOnly(part==DocPosition::Source);
    setUndoRedoEnabled(false);
    setAcceptRichText(false);

    if (part==DocPosition::Target)
    {
        connect (document(), SIGNAL(contentsChange(int,int,int)), this, SLOT(contentsChanged(int,int,int)));
        connect (this,SIGNAL(cursorPositionChanged()), this, SLOT(emitCursorPositionChanged()));
    }
}

void XliffTextEdit::reflectApprovementState()
{
    if (m_part==DocPosition::Source || m_currentPos.entry==-1)
        return;

    bool approved=m_catalog->isApproved(m_currentPos.entry);

    m_highlighter->setApprovementState(approved);
    m_highlighter->rehighlight();
    viewport()->setBackgroundRole(approved?QPalette::Base:QPalette::AlternateBase);


    if (approved) emit approvedEntryDisplayed();
    else          emit nonApprovedEntryDisplayed();

    bool untr=m_catalog->isUntranslated(m_currentPos);
    if (untr)     emit untranslatedEntryDisplayed();
    else          emit translatedEntryDisplayed();
}

void XliffTextEdit::reflectUntranslatedState()
{
    if (m_part==DocPosition::Source || m_currentPos.entry==-1)
        return;

    bool untr=m_catalog->isUntranslated(m_currentPos);
    if (untr)     emit untranslatedEntryDisplayed();
    else          emit translatedEntryDisplayed();
}


/**
 * makes MsgEdit reflect current entry
 **/
CatalogString XliffTextEdit::showPos(DocPosition docPosition, const CatalogString& refStr, bool keepCursor)
{
    docPosition.part=m_part;
    m_currentPos=docPosition;
    //kWarning()<<"called";

    CatalogString catalogString=m_catalog->catalogString(m_currentPos);
    QString target=catalogString.string;
    _oldMsgstr=target;

    //BEGIN pos
    QTextCursor cursor=textCursor();
    int pos=cursor.position();
    int anchor=cursor.anchor();
    kWarning()<<"called"<<"pos"<<pos<<anchor<<"keepCursor"<<keepCursor;
    if (!keepCursor && toPlainText()!=target)
    {
        kWarning()<<"resetting pos";
        pos=0;
        anchor=0;
    }
    //END pos

    disconnect (document(), SIGNAL(contentsChange(int,int,int)), this, SLOT(contentsChanged(int,int,int)));
    if (docPosition.part==DocPosition::Source)
        setContent(catalogString);
    else
        setContent(catalogString,refStr.string.isEmpty()?m_catalog->sourceWithTags(docPosition):refStr);
    connect (document(), SIGNAL(contentsChange(int,int,int)), this, SLOT(contentsChanged(int,int,int)));

    //BEGIN pos
    QTextCursor t=textCursor();
    t.movePosition(QTextCursor::Start);
    if (pos || anchor)
    {
        t.movePosition(QTextCursor::Start);
        kWarning()<<"setting"<<anchor<<pos;
        // I don't know why the following (more correct) code does not work
        t.movePosition(QTextCursor::NextCharacter,QTextCursor::MoveAnchor,anchor);
        int length=pos-anchor;
        if (length)
            t.movePosition(length<0?QTextCursor::PreviousCharacter:QTextCursor::NextCharacter,QTextCursor::KeepAnchor,qAbs(length));
    }
    setTextCursor(t);
    kWarning()<<"set?"<<textCursor().anchor()<<textCursor().position();
    //END pos

    reflectApprovementState();
    reflectUntranslatedState();
    return catalogString; //for the sake of not calling XliffStorage/doContent twice
}

void XliffTextEdit::setContent(const CatalogString& catStr, const CatalogString& refStr)
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

        TagRange tag=catStr.ranges.at(tagRangeIndex);
        QString name=' '+tag.id;
        QString text;
        if (sourceTagIdToIndex.isEmpty())
            text=QString::number(tagRangeIndex);
        else
            text=QString::number(sourceTagIdToIndex.value(tag.id));
        if (tag.start!=tag.end)
        {
            //kWarning()<<"b"<<i;
            if (tag.start==i)
            {
                //kWarning()<<"\t\tstart:"<<tag.getElementName()<<tag.id<<tag.start;
                text.append(" {");
                name.append("-start");
            }
            else
            {
                //kWarning()<<"\t\tend:"<<tag.getElementName()<<tag.id<<tag.end;
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
    m_highlighter->rehighlight(); //explicitly because we disabled signals
}



void XliffTextEdit::contentsChanged(int offset, int charsRemoved, int charsAdded)
{
    kWarning()<<"called";
    //kWarning()<<"!!!!!!!!!!!! offset"<<offset<<"charsRemoved"<<charsRemoved<<"_oldMsgstr"<<_oldMsgstr;
    QString editText=toPlainText();
    if (KDE_ISUNLIKELY( m_currentPos.entry==-1 || editText==_oldMsgstr ))
    {
        kWarning()<<"stop";
        return;
    }

    DocPosition pos=m_currentPos;
    pos.offset=offset;
    //kWarning()<<"offset"<<offset<<"charsRemoved"<<charsRemoved<<"_oldMsgstr"<<_oldMsgstr;


    QString target=m_catalog->targetWithTags(pos).string;
//BEGIN XLIFF markup handling
    //protect from tag removal
    bool markupRemoved=charsRemoved && target.mid(offset,charsRemoved).contains(TAGRANGE_IMAGE_SYMBOL);
    bool markupAdded=charsAdded && editText.mid(offset,charsAdded).contains(TAGRANGE_IMAGE_SYMBOL);
    if (markupRemoved || markupAdded)
    {
        bool modified=false;
        CatalogString targetWithTags=m_catalog->targetWithTags(m_currentPos);
        //special case when the user presses Del w/o selection
        if (!charsAdded && charsRemoved==1)
        {
            int i=targetWithTags.ranges.size();
            while(--i>=0)
            {
                if (targetWithTags.ranges.at(i).start==offset || targetWithTags.ranges.at(i).end==offset)
                {
                    modified=true;
                    pos.offset=targetWithTags.ranges.at(i).start;
                    m_catalog->push(new DelTagCmd(m_catalog,pos));
                }
            }
        }
        else if (!markupAdded) //check if all { plus } tags were selected
        {
            modified=removeTargetSubstring(offset, charsRemoved, /*refresh*/false);
            if (modified&&charsAdded)
                m_catalog->push(new InsTextCmd(m_catalog,pos,editText.mid(offset,charsAdded)));
        }

        kWarning()<<"calling showPos";
        showPos(m_currentPos,CatalogString(),/*keepCursor*/true);
        if (!modified)
        {
            kWarning()<<"stop";
            return;
        }
    }
//END XLIFF markup handling
    else
    {

        if (charsRemoved)
            m_catalog->push(new DelTextCmd(m_catalog,pos,_oldMsgstr.mid(offset,charsRemoved)));

        _oldMsgstr=editText;//newStr becomes OldStr
        //kWarning()<<"char"<<editText[offset].unicode();
        if (charsAdded)
            m_catalog->push(new InsTextCmd(m_catalog,pos,editText.mid(offset,charsAdded)));
    }

/* TODO
    if (_leds)
    {
        if (m_catalog->msgstr(pos).isEmpty()) _leds->ledUntr->on();
        else _leds->ledUntr->off();
    }
*/
    if (!m_catalog->isApproved(m_currentPos.entry)&&Settings::autoApprove())
        toggleApprovement(true);
    reflectUntranslatedState();

    // for mergecatalog (remove entry from index)
    // and for statusbar
    emit contentsModified(m_currentPos);
    kWarning()<<"finish";
}


// tagPlaces - pos -> int:
// >0 if both start and end parts of tag were deleted
// 1 means this is start, 2 means this is end
static bool fillTagPlaces(QMap<int,int>& tagPlaces,
                          const CatalogString& catalogString,
                          int start,
                          int len
                          )
{
    QString target=catalogString.string;
    if (len==-1)
        len=target.size();

    int t=start;
    while ((t=target.indexOf(TAGRANGE_IMAGE_SYMBOL,t))!=-1 && t<(start+len))
        tagPlaces[t++]=false;


    int i=catalogString.ranges.size();
    while(--i>=0)
    {
        qWarning()<<catalogString.ranges.at(i).getElementName();
        if (tagPlaces.contains(catalogString.ranges.at(i).start)
            &&tagPlaces.contains(catalogString.ranges.at(i).end))
        {
            qWarning()<<"start"<<catalogString.ranges.at(i).start<<"end"<<catalogString.ranges.at(i).end;
            tagPlaces[catalogString.ranges.at(i).end]=2;
            tagPlaces[catalogString.ranges.at(i).start]=1;
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

    return it==tagPlaces.constEnd();
}

bool XliffTextEdit::removeTargetSubstring(int delStart, int delLen, bool refresh)
{
    if (KDE_ISUNLIKELY( m_currentPos.entry==-1 ))
        return false;
    kWarning()<<"!!!!!!! called with"<<delStart<<delLen;

    CatalogString targetWithTags=m_catalog->targetWithTags(m_currentPos);
    QString target=targetWithTags.string;
    if (target.isEmpty())
        return false;

    QMap<int,int> tagPlaces;
    if (!fillTagPlaces(tagPlaces,targetWithTags,delStart,delLen))
        return false;

    int lenDecrement=0;

    m_catalog->beginMacro(i18nc("@item Undo action item","Remove text with markup"));

    //all indexes are ok (or target is just plain text)
    //modified=true;
    kWarning()<<"all indexes are ok";
    QMap<int,int>::const_iterator it = tagPlaces.constBegin();
    DocPosition pos=m_currentPos;
    while (it != tagPlaces.constEnd())
    {
        if (it.value()==1)
        {
            kWarning()<<"\t"<<it.key();
            pos.offset=it.key()-lenDecrement;
            DelTagCmd* cmd=new DelTagCmd(m_catalog,pos);
            m_catalog->push(cmd);
            lenDecrement+=1+cmd->tag().isPaired();
            //qWarning()<<"lenDecrement"<<lenDecrement;
        }
        ++it;
    }
    //charsRemoved-=lenDecrement;
    //qWarning()<<"charsRemoved"<<charsRemoved<<"offset"<<delStart;
    pos.offset=delStart;
    if (delLen)
    {
        QString rText=target.mid(delStart,delLen);
        rText.remove(TAGRANGE_IMAGE_SYMBOL);
        qWarning()<<"rText"<<rText<<"delStart"<<delStart;
        if (!rText.isEmpty())
            m_catalog->push(new DelTextCmd(m_catalog,pos,rText));
    }

    m_catalog->endMacro();


    if (!m_catalog->isApproved(m_currentPos.entry))
        toggleApprovement(true);

    if (refresh)
    {
        kWarning()<<"calling showPos";
        showPos(m_currentPos,CatalogString(),/*keepCursor*/true/*false*/);
    }
    emit contentsModified(m_currentPos.entry);
    return true;
}

void XliffTextEdit::insertCatalogString(const CatalogString& catStr, int start, bool refresh)
{
    m_catalog->beginMacro(i18nc("@item Undo action item","Insert text with markup"));
    QMap<int,int> posToTagRange;
    int i=catStr.ranges.size();
    //if (i) kWarning()<<"tags we got:";
    while(--i>=0)
    {
        //kWarning()<<"\t"<<catStr.ranges.at(i).getElementName()<<catStr.ranges.at(i).id<<catStr.ranges.at(i).start<<catStr.ranges.at(i).end;
        posToTagRange.insert(catStr.ranges.at(i).start,i);
        posToTagRange.insert(catStr.ranges.at(i).end,i);
    }

    DocPosition pos=m_currentPos;
    i=0;
    int prev=0;
    while ((i = catStr.string.indexOf(TAGRANGE_IMAGE_SYMBOL, i)) != -1)
    {
        qWarning()<<i<<catStr.string.left(i);
        //text that was before tag we found
        if (i-prev)
        {
            pos.offset=start+prev;
            m_catalog->push(new InsTextCmd(m_catalog,pos,catStr.string.mid(prev,i-prev)));
        }

        //now dealing with tag
        TagRange tag=catStr.ranges.at(posToTagRange.value(i));
        qWarning()<<i<<"testing for tag"<<tag.name()<<tag.start<<tag.start;
        if (tag.start==i) //this is an opening tag (may be single tag)
        {
            pos.offset=start+i;
            tag.start+=start;
            tag.end+=start;
            m_catalog->push(new InsTagCmd(m_catalog,pos,tag));
        }
        prev=++i;
    }
    pos.offset=start+prev;
    if (catStr.string.size()-pos.offset)
        m_catalog->push(new InsTextCmd(m_catalog,pos,catStr.string.mid(prev)));
    m_catalog->endMacro();

    if (refresh)
    {
        kWarning()<<"calling showPos";
        showPos(m_currentPos,CatalogString(),/*keepCursor*/true/*false*/);
        QTextCursor cursor=textCursor();
        cursor.movePosition(QTextCursor::NextCharacter,QTextCursor::MoveAnchor,catStr.string.size());
        setTextCursor(cursor);
    }
}



QMimeData* XliffTextEdit::createMimeDataFromSelection() const
{
    QMimeData *mimeData = new QMimeData;

    CatalogString catalogString=m_catalog->catalogString(m_currentPos);

    QTextCursor cursor=textCursor();
    int start=qMin(cursor.anchor(),cursor.position());
    int end=qMax(cursor.anchor(),cursor.position());

    QMap<int,int> tagPlaces;
    if (fillTagPlaces(tagPlaces,catalogString,start,end-start))
    {
        //transform CatalogString
        //TODO substring method
        catalogString.string=catalogString.string.mid(start,end-start);

        QList<TagRange>::iterator it=catalogString.ranges.begin();
        while (it != catalogString.ranges.end())
        {
            if (!tagPlaces.contains(it->start))
                it=catalogString.ranges.erase(it);
            else
            {
                it->start-=start;
                it->end-=start;
                ++it;
            }
        }

        QByteArray a;
        QDataStream out(&a,QIODevice::WriteOnly);
        QVariant v;
        qVariantSetValue<CatalogString>(v,catalogString);
        out<<v;
        mimeData->setData("application/x-lokalize-xliff+xml",a);
    }

    QString text=catalogString.string;
    text.remove(TAGRANGE_IMAGE_SYMBOL);
    mimeData->setText(text);
    return mimeData;
}

void XliffTextEdit::insertFromMimeData(const QMimeData* source)
{
    if (m_part==DocPosition::Source)
        return;

    if (source->hasFormat("application/x-lokalize-xliff+xml"))
    {
        QVariant v;
        QDataStream in(&(source->data("application/x-lokalize-xliff+xml")),QIODevice::ReadOnly);
        in>>v;
        qWarning()<<"ins"<<qVariantValue<CatalogString>(v).string<<qVariantValue<CatalogString>(v).ranges.size();

        kWarning()<<"pos"<<textCursor().position();
        //sets right cursor position implicitly
        QMimeData mimeData;
        mimeData.setText("");
        KTextEdit::insertFromMimeData(&mimeData);
        kWarning()<<"pos"<<textCursor().position();

        insertCatalogString(qVariantValue<CatalogString>(v), textCursor().position());
    }
    else
    {
        QString text=source->text();
        text.remove(TAGRANGE_IMAGE_SYMBOL);
        insertPlainText(text);
    }
}


















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

void XliffTextEdit::keyPressEvent(QKeyEvent *keyEvent)
{
    if (m_part==DocPosition::Source)
        return KTextEdit::keyPressEvent(keyEvent);

    static QString spclChars("abfnrtv'\"?\\");

    //BEGIN GENERAL
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
    //END GENERAL



    else if(keyEvent->matches(QKeySequence::MoveToPreviousPage))
        emit gotoPrevRequested();
    else if(keyEvent->matches(QKeySequence::MoveToNextPage))
        emit gotoNextRequested();
    else if(keyEvent->matches(QKeySequence::Undo))
        emit undoRequested();
    else if(keyEvent->matches(QKeySequence::Redo))
        emit redoRequested();
    else if (keyEvent->modifiers() == (Qt::AltModifier|Qt::ControlModifier))
    {
        if(keyEvent->key()==Qt::Key_Home)
            emit gotoFirstRequested();
        else if(keyEvent->key()==Qt::Key_End)
            emit gotoLastRequested();
    }
/*    else if (!keyEvent->modifiers()&&(keyEvent->key()==Qt::Key_Backspace||keyEvent->key()==Qt::Key_Delete))
    {
        //only for cases when:
        //-BkSpace was hit and cursor was atStart
        //-Del was hit and cursor was atEnd
        if (KDE_ISUNLIKELY( !m_catalog->isApproved(m_currentPos.entry) && !textCursor().hasSelection() ))
                          && (textCursor().atStart()||textCursor().atEnd()) ))
            ;//toggleApprovement(true); TODO
    }*/
    //clever editing
    else if(keyEvent->key()==Qt::Key_Return||keyEvent->key()==Qt::Key_Enter)
    {
        QString str=toPlainText();
        QTextCursor t=textCursor();
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
        insertPlainText(ins);
    }
    else if( (keyEvent->modifiers()&Qt::ControlModifier)?
                (keyEvent->key()==Qt::Key_D) :
                (keyEvent->key()==Qt::Key_Delete)
                && textCursor().atEnd())
    {
        kWarning()<<"workaround for Qt/X11 bug";
        QTextCursor t=textCursor();
        if(!t.hasSelection())
        {
            int pos=t.position();
            QString str=toPlainText();
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
        setTextCursor(t);
    }
    else if( (!keyEvent->modifiers()&&keyEvent->key()==Qt::Key_Backspace)
            || ( ( keyEvent->modifiers() & Qt::ControlModifier ) && keyEvent->key() == Qt::Key_H ) )
    {
        QTextCursor t=textCursor();
        if(!t.hasSelection())
        {
            int pos=t.position();
            QString str=toPlainText();
            if(!str.isEmpty() && pos>0 && spclChars.contains(str.at(pos-1)))
            {
                if(pos>1 && str.at(pos-2)=='\\' && !isMasked(str,pos-2))
                {
                    //TODO check for imag
                    t.deletePreviousChar();
                    t.deletePreviousChar();
                    setTextCursor(t);
                    kWarning()<<"set-->"<<textCursor().anchor()<<textCursor().position();
                }
            }

        }
        KTextEdit::keyPressEvent(keyEvent);
    }
    else if(keyEvent->text()=="\"")
    {
        QTextCursor t=textCursor();
        int pos=t.position();
        QString str=toPlainText();
        QString ins=QChar('\"');

        if(pos==0 || str.at(pos-1)!='\\' || isMasked(str,pos-1))
            ins.prepend('\\');

        t.insertText(ins);
        setTextCursor(t);
    }
    else if(keyEvent->key() == Qt::Key_Tab)
        insertPlainText("\\t");
    else if( keyEvent->key() == Qt::Key_Space && ( keyEvent->modifiers() & Qt::AltModifier ) )
        insertPlainText(QChar(0x00a0U));
    else
        KTextEdit::keyPressEvent(keyEvent);
}

void XliffTextEdit::keyReleaseEvent(QKeyEvent* e)
{
    if ( (e->key()==Qt::Key_Alt) && m_currentUnicodeNumber >= 32 )
    {
        insertPlainText(QChar( m_currentUnicodeNumber ));
        m_currentUnicodeNumber=0;
    }
    else
        KTextEdit::keyReleaseEvent(e);
}

QString XliffTextEdit::toPlainText()
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


void XliffTextEdit::toggleApprovement(bool approved)
{
    //kWarning()<<"called";
    if (KDE_ISUNLIKELY( m_currentPos.entry==-1 ))
        return;

    m_catalog->push(new ToggleApprovementCmd(m_catalog,m_currentPos.entry,approved));
    reflectApprovementState();
}



void XliffTextEdit::emitCursorPositionChanged()
{
    emit cursorPositionChanged(textCursor().columnNumber());
}



void XliffTextEdit::tagMenu()
{
    QMenu menu;
    QAction* txt=0;

    CatalogString sourceWithTags=m_catalog->sourceWithTags(m_currentPos);
    int count=sourceWithTags.ranges.size();
    if (count)
    {
        QMap<QString,int> tagIdToIndex=m_catalog->targetWithTags(m_currentPos).tagIdToIndex();
        bool hasActive=false;
        for (int i=0;i<count;++i)
        {
            //txt=menu.addAction(sourceWithTags.ranges.at(i));
            txt=menu.addAction(QString::number(i)/*+" "+sourceWithTags.ranges.at(i).id*/);
            txt->setData(QVariant(i));
            if (!hasActive && !tagIdToIndex.contains(sourceWithTags.ranges.at(i).id))
            {
                hasActive=true;
                menu.setActiveAction(txt);
            }
        }
        txt=menu.exec(mapToGlobal(cursorRect().bottomRight()));
        if (!txt) return;
        TagRange tag=sourceWithTags.ranges.at(txt->data().toInt());
        QTextCursor cursor=textCursor();
        tag.start=qMin(cursor.anchor(),cursor.position());
        tag.end=qMax(cursor.anchor(),cursor.position())+tag.isPaired();
        m_catalog->push(new InsTagCmd(m_catalog,m_currentPos,tag));
        showPos(m_currentPos,CatalogString(),/*keepCursor*/true);
        cursor.movePosition(QTextCursor::NextCharacter,QTextCursor::MoveAnchor,tag.end+1+tag.isPaired());
    }
    else
    {
        if (KDE_ISUNLIKELY( Project::instance()->markup().isEmpty() ))
            return;

        //QRegExp tag("(<[^>]*>)+|\\&\\w+\\;");
        QRegExp tag(Project::instance()->markup());
        tag.setMinimal(true);
        QString en=m_catalog->sourceWithTags(m_currentPos).string;
        QString target(toPlainText());
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
        txt=menu.exec(mapToGlobal(cursorRect().bottomRight()));
        if (txt)
            insertPlainText(txt->text());

    }
}


void XliffTextEdit::source2target()
{
    CatalogString sourceWithTags=m_catalog->sourceWithTags(m_currentPos);
    QString text=sourceWithTags.string;
    QString out;
    QString ctxt(m_catalog->msgctxt(m_currentPos.entry));

    //TODO ask for the fillment if the first time.
    //BEGIN KDE specific part
    if( ctxt.startsWith( "NAME OF TRANSLATORS" ) || text.startsWith( "_: NAME OF TRANSLATORS\\n" ))
    {
        if (!document()->isEmpty())
            out=", ";
        out+=Settings::authorLocalizedName();
    }
    else if( ctxt.startsWith( "EMAIL OF TRANSLATORS" ) || text.startsWith( "_: EMAIL OF TRANSLATORS\\n" ))
    {
        if (!document()->isEmpty())
            out=", ";
        out+=Settings::authorEmail();
    }
    else if( /*_catalog->isGeneratedFromDocbook() &&*/ text.startsWith( "ROLES_OF_TRANSLATORS" ) )
    {
        if (!document()->isEmpty())
            out='\n';
        out+="<othercredit role=\\\"translator\\\">\n"
        "<firstname></firstname><surname></surname>\n"
        "<affiliation><address><email>"+Settings::authorEmail()+"</email></address>\n"
        "</affiliation><contrib></contrib></othercredit>";
    }
    else if( text.startsWith( "CREDIT_FOR_TRANSLATORS" ) )
    {
        if (!document()->isEmpty())
            out='\n';
        out+="<para>"+Settings::authorLocalizedName()+'\n'+
            "<email>"+Settings::authorEmail()+"</email></para>";
    }
    //END KDE specific part


    if (out.isEmpty())
    {
        m_catalog->beginMacro(i18nc("@item Undo action item","Copy source to target"));
        DocPosition pos=m_currentPos;pos.offset=0;
        removeTargetSubstring(0,-1,/*refresh*/false);
        insertCatalogString(sourceWithTags,0,/*refresh*/false);
        m_catalog->endMacro();

        showPos(m_currentPos,sourceWithTags,/*keepCursor*/false);

        if (KDE_ISUNLIKELY( !m_catalog->isApproved(pos.entry)&&Settings::autoApprove() ))
            toggleApprovement(true);
    }
    else
    {
        QTextCursor t=textCursor();
        t.movePosition(QTextCursor::End);
        t.insertText(out);
        setTextCursor(t);
    }
}



#include "xlifftextedit.moc"
