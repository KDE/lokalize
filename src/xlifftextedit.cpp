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

#undef KDE_NO_DEBUG_OUTPUT

#include "xlifftextedit.h"
#include "catalog.h"
#include "cmd.h"
#include "syntaxhighlighter.h"
#include "prefs_lokalize.h"
#include "prefs.h"
#include "project.h"
#include "completionstorage.h"

#include <kcompletionbox.h>

#include <QStringBuilder>
#include <QPixmap>
#include <QPushButton>
#include <QPainter>
#include <QStyle>
#include <QApplication>
#include <QStyleOptionButton>
#include <QMimeData>
#include <QMetaType>

#include <QMenu>
#include <QMouseEvent>
#include <QToolTip>

// static int im_count=0;
// static int im_time=0;

#undef KDE_NO_DEBUG_OUTPUT
#include <QScrollBar>


inline static QImage generateImage(const QString& str, const QFont& font)
{
    //     im_count++;
    //     QTime a;a.start();

    QStyleOptionButton opt;
    opt.fontMetrics=QFontMetrics(font);
    opt.text=' '+str+' ';
    opt.rect=opt.fontMetrics.boundingRect(opt.text).adjusted(0,0,5,5);
    opt.rect.moveTo(0,0);

    QImage result(opt.rect.size(),QImage::Format_ARGB32);
    result.fill(0);//0xAARRGGBB
    QPainter painter(&result);
    QApplication::style()->drawControl(QStyle::CE_PushButton,&opt,&painter);

    //     im_time+=a.elapsed();
    //     kWarning()<<im_count<<im_time;
    return result;
}

class MyCompletionBox: public KCompletionBox
{
public:
    MyCompletionBox(QWidget* p):KCompletionBox(p){}
    QSize sizeHint() const;

    bool eventFilter(QObject* , QEvent* ); //reimplemented to deliver more keypresses to XliffTextEdit
};

QSize MyCompletionBox::sizeHint() const
{
    int h = count()?(sizeHintForRow(0)):0;
    h=qMin(count()*h,10*h) + 2*frameWidth();
    int w = sizeHintForColumn(0) + verticalScrollBar()->width() + 2*frameWidth();
    return QSize(w, h);
}

bool MyCompletionBox::eventFilter(QObject* object, QEvent* event)
{
    if (event->type()==QEvent::KeyPress)
    {
        QKeyEvent* e = static_cast<QKeyEvent*>(event);
        if (e->key()==Qt::Key_PageDown || e->key()==Qt::Key_PageUp)
        {
            hide();
            return false;
        }
    }
    return KCompletionBox::eventFilter(object, event);
}


#if 1
class XliffTextEditSpellInterface: public KTextEditSpellInterface
{
public:
    XliffTextEditSpellInterface(SyntaxHighlighter* highlighter);
    ~XliffTextEditSpellInterface(){};

    bool isSpellCheckingEnabled() const {return m_enabled;}
    void setSpellCheckingEnabled(bool enable);
    bool shouldBlockBeSpellChecked(const QString &block) const{Q_UNUSED(block); return true;}
private:
    bool m_enabled;
    SyntaxHighlighter* m_highlighter;
};

XliffTextEditSpellInterface::XliffTextEditSpellInterface(SyntaxHighlighter* highlighter)
    : KTextEditSpellInterface()
    , m_enabled(Settings::autoSpellcheck())
    , m_highlighter(highlighter)
{
    m_highlighter->setActive(m_enabled);
}

void XliffTextEditSpellInterface::setSpellCheckingEnabled(bool enable)
{
    Settings::setAutoSpellcheck(enable);
    m_enabled=enable;
    m_highlighter->setActive(enable);
    SettingsController::instance()->dirty=true;
}
#endif

TranslationUnitTextEdit::TranslationUnitTextEdit(Catalog* catalog, DocPosition::Part part, QWidget* parent)
    : KTextEdit(parent)
    , m_currentUnicodeNumber(0)
    , m_langUsesSpaces(true)
    , m_catalog(catalog)
    , m_part(part)
    , m_highlighter(new SyntaxHighlighter(this))
    , m_completionBox(0)
{
    setReadOnly(part==DocPosition::Source);
    setUndoRedoEnabled(false);
    setAcceptRichText(false);

    if (part==DocPosition::Target)
    {
        connect (document(), SIGNAL(contentsChange(int,int,int)), this, SLOT(contentsChanged(int,int,int)));
        connect (this,SIGNAL(cursorPositionChanged()), this, SLOT(emitCursorPositionChanged()));
    }
    connect (catalog,SIGNAL(signalFileLoaded()), this, SLOT(fileLoaded()));
    //connect (Project::instance(),SIGNAL(configChanged()), this, SLOT(projectConfigChanged()));

    setSpellInterface(new XliffTextEditSpellInterface(m_highlighter));
    setHighlighter(m_highlighter);
}

void TranslationUnitTextEdit::fileLoaded()
{
    QString langCode=m_part==DocPosition::Source? m_catalog->sourceLangCode():m_catalog->targetLangCode();

    QLocale langLocale(langCode);
    m_highlighter->setCurrentLanguage(langLocale.name());

    //"i use an english locale while translating kde pot files from english to hebrew" Bug #181989
    Qt::LayoutDirection targetLanguageDirection=Qt::LeftToRight;
    static QLocale::Language rtlLanguages[]={QLocale::Arabic, QLocale::Hebrew, QLocale::Urdu, QLocale::Persian, QLocale::Pashto};
    int i=sizeof(rtlLanguages)/sizeof(QLocale::Arabic);
    while (--i>=0 && langLocale.language()!=rtlLanguages[i])
        ;
    if (i!=-1)
        targetLanguageDirection=Qt::RightToLeft;
    setLayoutDirection(targetLanguageDirection);

    if (m_part==DocPosition::Source)
        return;

    //"Some language do not need space between words. For example Chinese."
    static QLocale::Language noSpaceLanguages[]={QLocale::Chinese};
    i=sizeof(noSpaceLanguages)/sizeof(QLocale::Chinese);
    while (--i>=0 && langLocale.language()!=noSpaceLanguages[i])
        ;
    m_langUsesSpaces=(i==-1);
}

void TranslationUnitTextEdit::reflectApprovementState()
{
    if (m_part==DocPosition::Source || m_currentPos.entry==-1)
        return;

    bool approved=m_catalog->isApproved(m_currentPos.entry);

    m_highlighter->setApprovementState(approved);
    disconnect (document(), SIGNAL(contentsChange(int,int,int)), this, SLOT(contentsChanged(int,int,int)));
    m_highlighter->rehighlight();
    connect (document(), SIGNAL(contentsChange(int,int,int)), this, SLOT(contentsChanged(int,int,int)));
    viewport()->setBackgroundRole(approved?QPalette::Base:QPalette::AlternateBase);


    if (approved) emit approvedEntryDisplayed();
    else          emit nonApprovedEntryDisplayed();

    bool untr=m_catalog->isEmpty(m_currentPos);
    if (untr)     emit untranslatedEntryDisplayed();
    else          emit translatedEntryDisplayed();
}

void TranslationUnitTextEdit::reflectUntranslatedState()
{
    if (m_part==DocPosition::Source || m_currentPos.entry==-1)
        return;

    bool untr=m_catalog->isEmpty(m_currentPos);
    if (untr)     emit untranslatedEntryDisplayed();
    else          emit translatedEntryDisplayed();
}


/**
 * makes MsgEdit reflect current entry
 **/
CatalogString TranslationUnitTextEdit::showPos(DocPosition docPosition, const CatalogString& refStr, bool keepCursor)
{
    docPosition.part=m_part;
    m_currentPos=docPosition;

    CatalogString catalogString=m_catalog->catalogString(m_currentPos);
    QString target=catalogString.string;
    _oldMsgstr=target;
    //_oldMsgstrAscii=document()->toPlainText(); <-- MOVED THIS TO THE END

    //BEGIN pos
    QTextCursor cursor=textCursor();
    int pos=cursor.position();
    int anchor=cursor.anchor();
    //kWarning()<<"called"<<"pos"<<pos<<anchor<<"keepCursor"<<keepCursor;
    if (!keepCursor && toPlainText()!=target)
    {
        //kWarning()<<"resetting pos";
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

    _oldMsgstrAscii=document()->toPlainText();

    //BEGIN pos
    QTextCursor t=textCursor();
    t.movePosition(QTextCursor::Start);
    if (pos || anchor)
    {
        //kWarning()<<"setting"<<anchor<<pos;
        // I don't know why the following (more correct) code does not work
        t.setPosition(anchor,QTextCursor::MoveAnchor);
        int length=pos-anchor;
        if (length)
            t.movePosition(length<0?QTextCursor::PreviousCharacter:QTextCursor::NextCharacter,QTextCursor::KeepAnchor,qAbs(length));
    }
    setTextCursor(t);
    //kWarning()<<"set?"<<textCursor().anchor()<<textCursor().position();
    //END pos

    reflectApprovementState();
    reflectUntranslatedState();
    return catalogString; //for the sake of not calling XliffStorage/doContent twice
}

void TranslationUnitTextEdit::setContent(const CatalogString& catStr, const CatalogString& refStr)
{
    //kWarning()<<"";
    //kWarning()<<"START";
    //kWarning()<<str<<ranges.size();
    //prevent undo tracking system from recording this 'action'
    document()->blockSignals(true);
    clear();

    QTextCursor c=textCursor();
    insertContent(c,catStr,refStr);

    document()->blockSignals(false);

    if (m_part==DocPosition::Target)
        m_highlighter->setSourceString(refStr.string);
    else
        //reflectApprovementState() does this for Target
        m_highlighter->rehighlight(); //explicitly because we disabled signals
}

#if 0
struct SearchFunctor
{
    virtual int operator()(const QString& str, int startingPos);
};

int SearchFunctor::operator()(const QString& str, int startingPos)
{
    return str.indexOf(TAGRANGE_IMAGE_SYMBOL, startingPos);
}

struct AlternativeSearchFunctor: public SearchFunctor
{
    int operator()(const QString& str, int startingPos);
};

int AlternativeSearchFunctor::operator()(const QString& str, int startingPos)
{
    int tagPos=str.indexOf(TAGRANGE_IMAGE_SYMBOL, startingPos);
    int diffStartPos=str.indexOf("{KBABEL", startingPos);
    int diffEndPos=str.indexOf("{/KBABEL", startingPos);

    int diffPos=qMin(diffStartPos,diffEndPos);
    if (diffPos==-1)
        diffPos=qMax(diffStartPos,diffEndPos);

    int result=qMin(tagPos,diffPos);
    if (result==-1)
        result=qMax(tagPos,diffPos);
    return result;
}
#endif

void insertContent(QTextCursor& cursor, const CatalogString& catStr, const CatalogString& refStr, bool insertText)
{
    //settings for TMView
    QTextCharFormat chF=cursor.charFormat();
    QFont font=cursor.document()->defaultFont();
    //font.setWeight(chF.fontWeight());

    QMap<int,int> posToTag;
    int i=catStr.tags.size();
    //kDebug()<<"size:"<<i;
    while(--i>=0)
    {
        //kDebug()<<"\t"<<catStr.tags.at(i).getElementName()<<catStr.tags.at(i).id<<catStr.tags.at(i).start<<catStr.tags.at(i).end;
        posToTag.insert(catStr.tags.at(i).start,i);
        posToTag.insert(catStr.tags.at(i).end,i);
    }

    QMap<QString,int> sourceTagIdToIndex=refStr.tagIdToIndex();
    int refTagIndexOffset=sourceTagIdToIndex.size();

    i=0;
    int prev=0;
    while ((i = catStr.string.indexOf(TAGRANGE_IMAGE_SYMBOL, i)) != -1)
    {
#if 0
    SearchFunctor nextStopSymbol=AlternativeSearchFunctor();
    char state='0';
    while ((i = nextStopSymbol(catStr.string, i)) != -1)
    {
        //handle diff display for TMView
        if (catStr.string.at(i)!=TAGRANGE_IMAGE_SYMBOL)
        {
            if (catStr.string.at(i+1)=='/')
                state='0';
            else if (catStr.string.at(i+8)=='D')
                state='-';
            else
                state='+';
            continue;
        }
#endif
        if (insertText)
            cursor.insertText(catStr.string.mid(prev,i-prev));
        else
        {
            cursor.movePosition(QTextCursor::NextCharacter,QTextCursor::MoveAnchor,i-prev);
            cursor.deleteChar();//delete TAGRANGE_IMAGE_SYMBOL to insert it properly
        }

        if (!posToTag.contains(i))
        {
            prev=++i;
            continue;
        }
        int tagIndex=posToTag.value(i);
        InlineTag tag=catStr.tags.at(tagIndex);
        QString name=tag.id;
        QString text;
        if (tag.type==InlineTag::mrk)
            text="*";
        else if (!tag.equivText.isEmpty())
            text=tag.equivText; //TODO add number? when? -- right now this is done for gettext qt's 156 mark
        else
            text=QString::number(sourceTagIdToIndex.contains(tag.id)?sourceTagIdToIndex.value(tag.id):(tagIndex+refTagIndexOffset));
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
        if (cursor.document()->resource(QTextDocument::ImageResource, QUrl(name)).isNull())
            cursor.document()->addResource(QTextDocument::ImageResource, QUrl(name), generateImage(text,font));
        cursor.insertImage(name);//NOTE what if twice the same name?
        cursor.setCharFormat(chF);

        prev=++i;
    }
    cursor.insertText(catStr.string.mid(prev));
}



void TranslationUnitTextEdit::contentsChanged(int offset, int charsRemoved, int charsAdded)
{
    //kWarning()<<"contentsChanged. offset"<<offset<<"charsRemoved"<<charsRemoved<<"charsAdded"<<charsAdded<<"_oldMsgstr"<<_oldMsgstr;
    //HACK to workaround #218246
    const QString& editTextAscii=document()->toPlainText();
    if (editTextAscii==_oldMsgstrAscii)
    {
        //kWarning()<<"stopping"<<editTextAscii<<_oldMsgstrAscii;
        return;
    }



    const QString& editText=toPlainText();
    if (KDE_ISUNLIKELY( m_currentPos.entry==-1 || editText==_oldMsgstr ))
    {
        //kWarning()<<"stopping"<<m_currentPos.entry<<editText<<_oldMsgstr;
        return;
    }

    DocPosition pos=m_currentPos;
    pos.offset=offset;
    //kWarning()<<"offset"<<offset<<"charsRemoved"<<charsRemoved<<"_oldMsgstr"<<_oldMsgstr;

    QString target=m_catalog->targetWithTags(pos).string;
    const QString& addedText=editText.mid(offset,charsAdded);

//BEGIN XLIFF markup handling
    //protect from tag removal
    //TODO use midRef when Qt 4.8 is in distros
    bool markupRemoved=charsRemoved && QString::fromRawData(target.unicode()+offset,charsRemoved).contains(TAGRANGE_IMAGE_SYMBOL);
    bool markupAdded=charsAdded && addedText.contains(TAGRANGE_IMAGE_SYMBOL);
    if (markupRemoved || markupAdded)
    {
        bool modified=false;
        CatalogString targetWithTags=m_catalog->targetWithTags(m_currentPos);
        //special case when the user presses Del w/o selection
        if (!charsAdded && charsRemoved==1)
        {
            int i=targetWithTags.tags.size();
            while(--i>=0)
            {
                if (targetWithTags.tags.at(i).start==offset || targetWithTags.tags.at(i).end==offset)
                {
                    modified=true;
                    pos.offset=targetWithTags.tags.at(i).start;
                    m_catalog->push(new DelTagCmd(m_catalog,pos));
                }
            }
        }
        else if (!markupAdded) //check if all { plus } tags were selected
        {
            modified=removeTargetSubstring(offset, charsRemoved, /*refresh*/false);
            if (modified&&charsAdded)
                m_catalog->push(new InsTextCmd(m_catalog,pos,addedText));
        }

        //kWarning()<<"calling showPos";
        showPos(m_currentPos,CatalogString(),/*keepCursor*/true);
        if (!modified)
        {
            //kWarning()<<"stop";
            return;
        }
    }
//END XLIFF markup handling
    else
    {
        if (charsRemoved)
            m_catalog->push(new DelTextCmd(m_catalog,pos,_oldMsgstr.mid(offset,charsRemoved)));

        _oldMsgstr=editText;//newStr becomes OldStr
        _oldMsgstrAscii=editTextAscii;
        //kWarning()<<"char"<<editText[offset].unicode();
        if (charsAdded)
            m_catalog->push(new InsTextCmd(m_catalog,pos,addedText));

    }

/* TODO
    if (_leds)
    {
        if (m_catalog->msgstr(pos).isEmpty()) _leds->ledUntr->on();
        else _leds->ledUntr->off();
    }
*/
    requestToggleApprovement();
    reflectUntranslatedState();

    // for mergecatalog (remove entry from index)
    // and for statusbar
    emit contentsModified(m_currentPos);

    if (charsAdded==1)
    {
        int sp=target.lastIndexOf(CompletionStorage::instance()->rxSplit,offset-1);
        int len=(offset-sp);
        int wordCompletionLength=Settings::self()->wordCompletionLength();
        if (wordCompletionLength>=3 && len>=wordCompletionLength)
            doCompletion(offset+1);
        else if (m_completionBox)
                m_completionBox->hide();
    }
    else if (m_completionBox)
            m_completionBox->hide();

    //kWarning()<<"finish";
}


bool TranslationUnitTextEdit::removeTargetSubstring(int delStart, int delLen, bool refresh)
{
    if (KDE_ISUNLIKELY( m_currentPos.entry==-1 ))
        return false;

    if (!::removeTargetSubstring(m_catalog, m_currentPos, delStart, delLen))
        return false;

    requestToggleApprovement();

    if (refresh)
    {
        //kWarning()<<"calling showPos";
        showPos(m_currentPos,CatalogString(),/*keepCursor*/true/*false*/);
    }
    emit contentsModified(m_currentPos.entry);
    return true;
}

void TranslationUnitTextEdit::insertCatalogString(CatalogString catStr, int start, bool refresh)
{
    CatalogString source=m_catalog->sourceWithTags(m_currentPos);
    CatalogString target=m_catalog->targetWithTags(m_currentPos);


    QHash<QString,int> id2tagIndex;
    int i=source.tags.size();
    while(--i>=0)
        id2tagIndex.insert(source.tags.at(i).id,i);

    foreach(const InlineTag& tag, target.tags)
    {
        if (id2tagIndex.contains(tag.id))
            source.tags[id2tagIndex.value(target.tags.at(i).id)].id="REMOVEME";
    }

    //iterating from the end is essential
    i=source.tags.size();
    while(--i>=0)
        if (source.tags.at(i).id=="REMOVEME")
            source.tags.removeAt(i);


    adaptCatalogString(catStr,source);

    ::insertCatalogString(m_catalog,m_currentPos,catStr,start);

    if (refresh)
    {
        //kWarning()<<"calling showPos";
        showPos(m_currentPos,CatalogString(),/*keepCursor*/true/*false*/);
        QTextCursor cursor=textCursor();
        cursor.movePosition(QTextCursor::NextCharacter,QTextCursor::MoveAnchor,catStr.string.size());
        setTextCursor(cursor);
    }
}



QMimeData* TranslationUnitTextEdit::createMimeDataFromSelection() const
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

        QList<InlineTag>::iterator it=catalogString.tags.begin();
        while (it != catalogString.tags.end())
        {
            if (!tagPlaces.contains(it->start))
                it=catalogString.tags.erase(it);
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

void TranslationUnitTextEdit::insertFromMimeData(const QMimeData* source)
{
    if (m_part==DocPosition::Source)
        return;

    if (source->hasFormat("application/x-lokalize-xliff+xml"))
    {
        //kWarning()<<"has";
        QVariant v;
        QByteArray data=source->data("application/x-lokalize-xliff+xml");
        QDataStream in(&data,QIODevice::ReadOnly);
        in>>v;
        //qWarning()<<"ins"<<qVariantValue<CatalogString>(v).string<<qVariantValue<CatalogString>(v).ranges.size();

        int start=0;
        m_catalog->beginMacro(i18nc("@item Undo action item","Insert text with markup"));
        QTextCursor cursor=textCursor();
        if (cursor.hasSelection())
        {
            start=qMin(cursor.anchor(),cursor.position());
            int end=qMax(cursor.anchor(),cursor.position());
            removeTargetSubstring(start,end-start);
            cursor.setPosition(start);
            setTextCursor(cursor);
        }
        else
        //sets right cursor position implicitly -- needed for mouse paste
        {
            QMimeData mimeData;
            mimeData.setText("");
            KTextEdit::insertFromMimeData(&mimeData);
            start=textCursor().position();
        }

        insertCatalogString(qVariantValue<CatalogString>(v), start);
        m_catalog->endMacro();
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

static QString spclChars("abfnrtv'?\\");

void TranslationUnitTextEdit::keyPressEvent(QKeyEvent *keyEvent)
{
    if(keyEvent->matches(QKeySequence::MoveToPreviousPage))
        emit gotoPrevRequested();
    else if(keyEvent->matches(QKeySequence::MoveToNextPage))
        emit gotoNextRequested();
    else if(keyEvent->matches(QKeySequence::Undo))
        emit undoRequested();
    else if(keyEvent->matches(QKeySequence::Redo))
        emit redoRequested();
    else if(keyEvent->matches(QKeySequence::Find))
        emit findRequested();
    else if(keyEvent->matches(QKeySequence::FindNext))
        emit findNextRequested();
    else if(keyEvent->matches(QKeySequence::Replace))
        emit replaceRequested();
    else if (keyEvent->modifiers() == (Qt::AltModifier|Qt::ControlModifier))
    {
        if(keyEvent->key()==Qt::Key_Home)
            emit gotoFirstRequested();
        else if(keyEvent->key()==Qt::Key_End)
            emit gotoLastRequested();
    }
    else if (m_part==DocPosition::Source)
        return KTextEdit::keyPressEvent(keyEvent);

    //BEGIN GENERAL
    // ALT+123 feature TODO this is general so should be on another level
    else if( (keyEvent->modifiers()&Qt::AltModifier)
         &&!keyEvent->text().isEmpty()
         &&keyEvent->text().at(0).isDigit() )
    {
        QString text=keyEvent->text();
        while (!text.isEmpty()&& text.at(0).isDigit() )
        {
            m_currentUnicodeNumber = 10*m_currentUnicodeNumber+(text.at(0).digitValue());
            text.remove(0,1);
        }
        KTextEdit::keyPressEvent(keyEvent);
    }
    //END GENERAL

    else if (!keyEvent->modifiers()&&(keyEvent->key()==Qt::Key_Backspace||keyEvent->key()==Qt::Key_Delete))
    {
        //only for cases when:
        //-BkSpace was hit and cursor was atStart
        //-Del was hit and cursor was atEnd
        if (KDE_ISUNLIKELY( !m_catalog->isApproved(m_currentPos.entry) && !textCursor().hasSelection() )
                            && ((textCursor().atStart()&&keyEvent->key()==Qt::Key_Backspace)
                                 ||(textCursor().atEnd()&&keyEvent->key()==Qt::Key_Delete) ))
            requestToggleApprovement();
        else
            KTextEdit::keyPressEvent(keyEvent);
    }
    else if( keyEvent->key() == Qt::Key_Space && (keyEvent->modifiers()&Qt::AltModifier) )
        insertPlainText(QChar(0x00a0U));
    else if( keyEvent->key() == Qt::Key_Minus && (keyEvent->modifiers()&Qt::AltModifier) )
        insertPlainText(QChar(0x0000AD));
    else if (m_catalog->mimetype()!="text/x-gettext-translation")
        KTextEdit::keyPressEvent(keyEvent);
    //clever editing
    else if(keyEvent->key()==Qt::Key_Return||keyEvent->key()==Qt::Key_Enter)
    {
        if (m_completionBox&&m_completionBox->isVisible())
        {
            if (m_completionBox->currentItem())
                completionActivated(m_completionBox->currentItem()->text());
            else
                kWarning()<<"avoided a crash. a case for bug 238835!";
            m_completionBox->hide();
            return;
        }
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
            if(m_langUsesSpaces
               &&pos>0
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
        {
            ins+='\n';
            insertPlainText(ins);
        }
        else
            KTextEdit::keyPressEvent(keyEvent);
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
                &&!isMasked(str,pos)
                &&pos<str.length()-1
                &&spclChars.contains(str.at(pos+1)))
            {
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
                    t.deletePreviousChar();
                    t.deletePreviousChar();
                    setTextCursor(t);
                    //kWarning()<<"set-->"<<textCursor().anchor()<<textCursor().position();
                }
            }

        }
        KTextEdit::keyPressEvent(keyEvent);
    }
    else if(keyEvent->key() == Qt::Key_Tab)
        insertPlainText("\\t");
    else
        KTextEdit::keyPressEvent(keyEvent);
}

void TranslationUnitTextEdit::keyReleaseEvent(QKeyEvent* e)
{
    if ( (e->key()==Qt::Key_Alt) && m_currentUnicodeNumber >= 32 )
    {
        insertPlainText(QChar( m_currentUnicodeNumber ));
        m_currentUnicodeNumber=0;
    }
    else
        KTextEdit::keyReleaseEvent(e);
}

QString TranslationUnitTextEdit::toPlainText()
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



void TranslationUnitTextEdit::emitCursorPositionChanged()
{
    emit cursorPositionChanged(textCursor().columnNumber());
}

void TranslationUnitTextEdit::insertTag(InlineTag tag)
{
    QTextCursor cursor=textCursor();
    tag.start=qMin(cursor.anchor(),cursor.position());
    tag.end=qMax(cursor.anchor(),cursor.position())+tag.isPaired();
    kWarning()<<(m_part==DocPosition::Source)<<tag.start<<tag.end;
    m_catalog->push(new InsTagCmd(m_catalog,currentPos(),tag));
    showPos(currentPos(),CatalogString(),/*keepCursor*/true);
    cursor.movePosition(QTextCursor::NextCharacter,QTextCursor::MoveAnchor,tag.end+1+tag.isPaired());
    setFocus();
}

int TranslationUnitTextEdit::strForMicePosIfUnderTag(QPoint mice, CatalogString& str, bool tryHarder)
{
    QTextCursor cursor=cursorForPosition(mice);
    int pos=cursor.position();
    str=m_catalog->catalogString(m_currentPos);
    if (pos==-1 || pos>=str.string.size()) return -1;
    //kWarning()<<"here1"<<str.string.at(pos)<<str.string.at(pos-1)<<str.string.at(pos+1);


//     if (pos>0)
//     {
//         cursor.movePosition(QTextCursor::Left);
//         mice.setX(mice.x()+cursorRect(cursor).width()/2);
//         pos=cursorForPosition(mice).position();
//     }

    if (str.string.at(pos)!=TAGRANGE_IMAGE_SYMBOL)
    {
        bool cont=tryHarder && --pos>=0 && str.string.at(pos)==TAGRANGE_IMAGE_SYMBOL;
        if (!cont)
            return -1;
    }

    int result=str.tags.size();
    while(--result>=0 && str.tags.at(result).start!=pos && str.tags.at(result).end!=pos)
        ;
    return result;
}

void TranslationUnitTextEdit::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button()==Qt::LeftButton)
    {
        CatalogString str;
        int pos=strForMicePosIfUnderTag(event->pos(),str);
        if (pos!=-1 && m_part==DocPosition::Source)
        {
            emit tagInsertRequested(str.tags.at(pos));
            event->accept();
            return;
        }
    }
    KTextEdit::mouseReleaseEvent(event);
}


void TranslationUnitTextEdit::contextMenuEvent(QContextMenuEvent *event)
{
    CatalogString str;
    int pos=strForMicePosIfUnderTag(event->pos(),str);
    if (pos!=-1)
    {
        QString xid=str.tags.at(pos).xid;

        if (!xid.isEmpty())
        {
            QMenu menu;
            int entry=m_catalog->unitById(xid);
            /* QAction* findUnit=menu.addAction(entry>=m_catalog->numberOfEntries()?
                            i18nc("@action:inmenu","Show the binary unit"):
                            i18nc("@action:inmenu","Go to the referenced entry")); */

            QAction* result=menu.exec(event->globalPos());
            if (result)
            {
                kWarning()<<entry<<xid;
                if (entry>=m_catalog->numberOfEntries())
                    emit binaryUnitSelectRequested(xid);
                else
                    emit gotoEntryRequested(DocPosition(entry));
                event->accept();
            }
            return;
        }
    }
    if (textCursor().hasSelection())
    {
        QMenu menu;
        // QAction* lookup=menu.addAction(i18nc("@action:inmenu","Lookup selected text in translation memory"));
        if (menu.exec(event->globalPos()))
            emit tmLookupRequested(m_part,textCursor().selectedText());
        return;
    }

    if (m_part!=DocPosition::Target)
        return;

    KTextEdit::contextMenuEvent(event);

#if 0
    QTextCursor wordSelectCursor=cursorForPosition(event->pos());
    wordSelectCursor.select(QTextCursor::WordUnderCursor);
    if (m_highlighter->isWordMisspelled(wordSelectCursor.selectedText()))
    {
        QMenu menu;
        QMenu suggestions;
        foreach(const QString& s, m_highlighter->suggestionsForWord(wordSelectCursor.selectedText()))
            suggestions.addAction(s);
        if (!suggestions.isEmpty())
        {
            QAction* answer=suggestions.exec(event->globalPos());
            if (answer)
            {
                m_catalog->beginMacro(i18nc("@item Undo action item","Replace text"));
                wordSelectCursor.insertText(answer->text());
                m_catalog->endMacro();
            }
        }
    }
#endif

//     QMenu menu;
//     QAction* spellchecking=menu.addAction();
//     event->accept();
}

void TranslationUnitTextEdit::wheelEvent(QWheelEvent *event)
{
    if (m_part==DocPosition::Source || !Settings::mouseWheelGo())
        return KTextEdit::wheelEvent(event);

    switch (event->modifiers())
    {
        case Qt::ControlModifier:
            if (event->delta() > 0)
                emit gotoPrevFuzzyRequested();
            else
                emit gotoNextFuzzyRequested();
            break;
        case Qt::AltModifier:
            if (event->delta() > 0)
                emit gotoPrevUntranslatedRequested();
            else
                emit gotoNextUntranslatedRequested();
            break;
        case Qt::ControlModifier + Qt::ShiftModifier:
            if (event->delta() > 0)
                emit gotoPrevFuzzyUntrRequested();
            else
                emit gotoNextFuzzyUntrRequested();
            break;
        case Qt::ShiftModifier:
            return KTextEdit::wheelEvent(event);
        default:
            if (event->delta() > 0)
                emit gotoPrevRequested();
            else
                emit gotoNextRequested();
    }
}

void TranslationUnitTextEdit::spellReplace()
{
    QTextCursor wordSelectCursor=textCursor();
    wordSelectCursor.select(QTextCursor::WordUnderCursor);
    if (!m_highlighter->isWordMisspelled(wordSelectCursor.selectedText()))
        return;

    const QStringList& suggestions=m_highlighter->suggestionsForWord(wordSelectCursor.selectedText());
    if (suggestions.isEmpty())
        return;

    m_catalog->beginMacro(i18nc("@item Undo action item","Replace text"));
    wordSelectCursor.insertText(suggestions.first());
    m_catalog->endMacro();
}

bool TranslationUnitTextEdit::event(QEvent *event)
{
    if (event->type()==QEvent::ToolTip)
    {
        QHelpEvent *helpEvent = static_cast<QHelpEvent *>(event);
        CatalogString str;
        int pos=strForMicePosIfUnderTag(helpEvent->pos(),str,true);
        if (pos!=-1)
        {
            QString tooltip=str.tags.at(pos).displayName();
            QToolTip::showText(helpEvent->globalPos(),tooltip);
            return true;
        }

        QString langCode=m_highlighter->currentLanguage();
        bool nospell=langCode.isEmpty();
        if (nospell)
            langCode=m_part==DocPosition::Source?m_catalog->sourceLangCode():m_catalog->targetLangCode();
        QString tip=KGlobal::locale()->languageCodeToName(langCode)%" ("%langCode%")";
        if (nospell)
            tip+=" - "%i18n("no spellcheck available");
        QToolTip::showText(helpEvent->globalPos(), tip);
    }
    return KTextEdit::event(event);
}



void TranslationUnitTextEdit::tagMenu()     {doTag(false);}
void TranslationUnitTextEdit::tagImmediate(){doTag(true);}

void TranslationUnitTextEdit::doTag(bool immediate)
{
    QMenu menu;
    QAction* txt=0;

    CatalogString sourceWithTags=m_catalog->sourceWithTags(m_currentPos);
    int count=sourceWithTags.tags.size();
    if (count)
    {
        QMap<QString,int> tagIdToIndex=m_catalog->targetWithTags(m_currentPos).tagIdToIndex();
        bool hasActive=false;
        for (int i=0;i<count;++i)
        {
            //txt=menu.addAction(sourceWithTags.ranges.at(i));
            txt=menu.addAction(QString::number(i)/*+" "+sourceWithTags.ranges.at(i).id*/);
            txt->setData(QVariant(i));
            if (!hasActive && !tagIdToIndex.contains(sourceWithTags.tags.at(i).id))
            {
                if (immediate)
                {
                    insertTag(sourceWithTags.tags.at(txt->data().toInt()));
                    return;
                }
                hasActive=true;
                menu.setActiveAction(txt);
            }
        }
        if (immediate) return;
        txt=menu.exec(mapToGlobal(cursorRect().bottomRight()));
        if (!txt) return;
        insertTag(sourceWithTags.tags.at(txt->data().toInt()));
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
            {
                if (immediate)
                {
                    insertPlainText(txt->text());
                    return;
                }
                menu.setActiveAction(txt);
            } else if (posInMsgStr!=-1)
                posInMsgStr+=tag.matchedLength();
        }
        if (!txt || immediate)
            return;

        //txt=menu.exec(_msgidEdit->mapToGlobal(QPoint(0,0)));
        txt=menu.exec(mapToGlobal(cursorRect().bottomRight()));
        if (txt)
            insertPlainText(txt->text());
    }
}


void TranslationUnitTextEdit::source2target()
{
    CatalogString sourceWithTags=m_catalog->sourceWithTags(m_currentPos);
    QString text=sourceWithTags.string;
    QString out;
    QString ctxt=m_catalog->context(m_currentPos.entry).first();
    QRegExp delimiter("\\s*,\\s*");

    //TODO ask for the fillment if the first time.
    //BEGIN KDE specific part
    if( ctxt.startsWith( "NAME OF TRANSLATORS" ) || text.startsWith( "_: NAME OF TRANSLATORS\\n" ))
    {
        if (!document()->toPlainText().split(delimiter).contains(Settings::authorLocalizedName())) {
            if (!document()->isEmpty())
                out=", ";
            out+=Settings::authorLocalizedName();
        }
    }
    else if( ctxt.startsWith( "EMAIL OF TRANSLATORS" ) || text.startsWith( "_: EMAIL OF TRANSLATORS\\n" )) {
        if (!document()->toPlainText().split(delimiter).contains(Settings::authorEmail())) {
            if (!document()->isEmpty())
                out=", ";
            out+=Settings::authorEmail();
        }
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

    else {
        m_catalog->beginMacro(i18nc("@item Undo action item","Copy source to target"));
        removeTargetSubstring(0,-1,/*refresh*/false);
        insertCatalogString(sourceWithTags,0,/*refresh*/false);
        m_catalog->endMacro();

        showPos(m_currentPos,sourceWithTags,/*keepCursor*/false);

        requestToggleApprovement();
    }
    if (!out.isEmpty()) {
        QTextCursor t=textCursor();
        t.movePosition(QTextCursor::End);
        t.insertText(out);
        setTextCursor(t);
    }
}

void TranslationUnitTextEdit::requestToggleApprovement()
{
    if (m_catalog->isApproved(m_currentPos.entry)||!Settings::autoApprove())
        return;

    bool skip=m_catalog->isPlural(m_currentPos);
    if (skip)
    {
        skip=false;
        DocPos pos(m_currentPos);
        for (pos.form=0;pos.form<m_catalog->numberOfPluralForms();++(pos.form))
            skip=skip||!m_catalog->isModified(pos);
    }
    if (!skip)
        emit toggleApprovementRequested();
}


void TranslationUnitTextEdit::cursorToStart()
{
    QTextCursor t=textCursor();
    t.movePosition(QTextCursor::Start);
    setTextCursor(t);
}


void TranslationUnitTextEdit::doCompletion(int pos)
{
    QTime a;a.start();
    QString target=m_catalog->targetWithTags(m_currentPos).string;
    int sp=target.lastIndexOf(CompletionStorage::instance()->rxSplit,pos-1);
    int len=(pos-sp)-1;

    QStringList s=CompletionStorage::instance()->makeCompletion(QString::fromRawData(target.unicode()+sp+1,len));

    if (!m_completionBox)
    {
//BEGIN creation
        m_completionBox=new MyCompletionBox(this);
        connect(m_completionBox,SIGNAL(activated(QString)),this,SLOT(completionActivated(QString)));
        m_completionBox->setSizePolicy(QSizePolicy::Maximum,QSizePolicy::Preferred);
//END creation
    }
    m_completionBox->setItems(s);
    if (s.size() && !s.first().isEmpty())
    {
        m_completionBox->setCurrentRow(0);
        //qApp->removeEventFilter( m_completionBox );
        if (!m_completionBox->isVisible()) //NOTE remove the ckeck if kdelibs gets adapted
            m_completionBox->show();
        m_completionBox->resize(m_completionBox->sizeHint());
        m_completionBox->move(viewport()->mapToGlobal(cursorRect().bottomRight()));
    }
    else
        m_completionBox->hide();
    kDebug()<<"hits generated in"<<a.elapsed()<<"msecs";
}

void TranslationUnitTextEdit::doExplicitCompletion()
{
    doCompletion(textCursor().anchor());
}

void TranslationUnitTextEdit::completionActivated(const QString& semiWord)
{
    QTextCursor cursor=textCursor();
    cursor.insertText(semiWord);
    setTextCursor(cursor);
}

#include "xlifftextedit.moc"
