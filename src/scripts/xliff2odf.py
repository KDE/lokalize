# -*- coding: utf-8 -*-
import Kross
import Lokalize
import Project
import Editor
import sys,os,re
from translate.convert import xliff2odf
from translate.storage import factory

Lokalize.busyCursor(True)

class XliffInput:
    def __init__(self, name, contents):
        self.name=name
        self.contents=contents
        self.read=lambda: contents

    def close(self): return

def convert():
    if not Lokalize.activeEditor() or Editor.currentFile()=='': return

    xliffpathname=Editor.currentFile()
    (path, filename)=os.path.split(xliffpathname)
    if not filename.endswith('.xlf'): return


    xliffinput=XliffInput(xliffpathname,Editor.currentFileContents())
    print xliffpathname

    store = factory.getobject(xliffpathname)
    odfpathname=store.getfilenames()[0]

    translatedodfpathname=os.path.splitext(odfpathname)[0]+'-'+Project.targetLangCode()+'.odt'


    print translatedodfpathname
    print odfpathname
    xliff2odf.convertxliff(xliffinput, translatedodfpathname, odfpathname)
    
    return translatedodfpathname

translatedodfpathname=convert()
print translatedodfpathname



if os.name=='nt': import socket  # only needed on win32-OOo3.0.0
import socket
import uno
import time

def show_in_ooo(translatedodfpathname):
    if len(translatedodfpathname)==0: return

    def establish_connection():
        localContext = uno.getComponentContext()
        resolver = localContext.ServiceManager.createInstanceWithContext("com.sun.star.bridge.UnoUrlResolver", localContext )
        return resolver.resolve( "uno:socket,host=localhost,port=2002;urp;StarOffice.ComponentContext" )

    try: ctx = establish_connection()
    except:
        os.system('soffice "-accept=socket,host=localhost,port=2002;urp;"')
        for c in range(30):
            time.sleep(1) #sleeps rule )))
            try:ctx = establish_connection()
            except: continue
            break

    print "file://"+translatedodfpathname
    try:
        print Editor.currentEntryId()
        paranumre=re.compile('/text:p\\[([0-9]*)\\]$')
        o=paranumre.search(Editor.currentEntryId())
        paranum=int(o.group(1))
        print paranum
    except:
        print 'error determining pos'
        #return ctx
    if 1:
    #try:
        desktop = ctx.ServiceManager.createInstanceWithContext( "com.sun.star.frame.Desktop",ctx)
        model = desktop.loadComponentFromURL( "file://"+translatedodfpathname,"_default", 0, () )

        dispatcher = ctx.ServiceManager.createInstanceWithContext( "com.sun.star.frame.DispatchHelper",ctx)
        dispatcher.executeDispatch(model.getCurrentController().getFrame(),".uno:Reload","",0,())

        #model = desktop.loadComponentFromURL( "file://"+translatedodfpathname,"_default", 0, () )
        #text = model.Text
        #cursor = text.createTextCursor()
        #cursor.gotoStart(False)
        #for i in range(paranum): cursor.gotoNextParagraph(False)

        #c=model.getCurrentController().getViewCursor()
        #c.gotoRange(cursor,False)
    #except:print 'error occured'

    return ctx

ctx=show_in_ooo(translatedodfpathname)
ctx.ServiceManager

Lokalize.busyCursor(False)
