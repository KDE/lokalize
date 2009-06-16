# -*- coding: utf-8 -*-
import sys,os,re
if os.name=='nt': import socket  # only needed on win32-OOo3.0.0

try: import uno
except:
    #opensuse needs this, while debian rocks w/o this
    sys.path.append('/usr/lib/ooo3/basis-link/program/')
    import uno

import time

def show_in_ooo(odfpathname,entryid):
    if len(odfpathname)==0: return

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
    print "file://"+odfpathname
    
    desktop = ctx.ServiceManager.createInstanceWithContext( "com.sun.star.frame.Desktop",ctx)
    model = desktop.loadComponentFromURL( "file://"+odfpathname,"_default", 0, () )

    dispatcher = ctx.ServiceManager.createInstanceWithContext( "com.sun.star.frame.DispatchHelper",ctx)
    dispatcher.executeDispatch(model.getCurrentController().getFrame(),".uno:Reload","",0,())

    model = desktop.loadComponentFromURL( "file://"+odfpathname,"_default", 0, () )
    #model = desktop.getCurrentComponent()

    cursor = model.Text.createTextCursor()
    cursor.gotoStart(False)
    try:
        print entryid
        #office:document-content[0]/office:body[0]/office:text[0]/text:h[0]
        standardstart='office:document-content[0]/office:body[0]/office:text[0]/'
        if entryid.startswith(standardstart): entryid=entryid[len(standardstart):]
        else: print 'non-standard start: %s' % entryid

        numre=re.compile('\\[([0-9]*)\\]')
        elemre=re.compile(':([^\\[]*)\\[')
        tableprops={}
        for pathcomponent in entryid.split('/'):
            paranum=int(numre.search(pathcomponent).group(1))
            elem=elemre.search(pathcomponent).group(1)
            if pathcomponent.startswith('text'):
                if elem=='p':
                    #office:document-content[0]/office:body[0]/office:text[0]/text:p[0]
                    for i in range(paranum): cursor.gotoNextParagraph(False)
            elif pathcomponent.startswith('table'):
                #office:document-content[0]/office:body[0]/office:text[0]/table:table[0]/table:table-row[0]/table:table-cell[0]/text:p[0]
                tableprops[elem]=paranum
                if len(tableprops.keys())==3:
                    cell=model.getTextTables().getByIndex(tableprops['table']).getCellByPosition(tableprops['table-cell'],tableprops['table-row'])
                    tableprops={}
                    cursor=cell.Text.createTextCursor()
                    cursor.gotoStart(False)


        c=model.getCurrentController().getViewCursor()
        c.gotoRange(cursor,False)

    except:
        print 'error determining pos'
        #return ctx

    ctx.ServiceManager


def main(argv=None):
    odfpathname=argv[1]
    entryid=argv[2]
    show_in_ooo(odfpathname,entryid)


if __name__ == '__main__':
    main(sys.argv)
