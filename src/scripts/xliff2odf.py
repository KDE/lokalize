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
    print Lokalize.activeEditor()
    print Editor.currentFile()
    if not Lokalize.activeEditor() or Editor.currentFile()=='': return

    xliffpathname=Editor.currentFile()
    (path, filename)=os.path.split(xliffpathname)
    print 'here'
    if not filename.endswith('.xlf'): return


    xliffinput=XliffInput(xliffpathname,Editor.currentFileContents())
    print 'xliffpathname',
    print xliffpathname

    store = factory.getobject(xliffpathname)
    odfpathname=store.getfilenames()[0]

    translatedodfpathname=os.path.splitext(odfpathname)[0]+'-'+Project.targetLangCode()+'.odt'


    print 'translatedodfpathname',
    print translatedodfpathname
    print 'odfpathname',
    print odfpathname
    xliff2odf.convertxliff(xliffinput, translatedodfpathname, odfpathname)
    
    return translatedodfpathname

translatedodfpathname=convert()
print 'translatedodfpathname: ',
print translatedodfpathname


print sys.path
ourPath=(filter(lambda p: os.path.exists(p+'/xliff2odf.py'),sys.path)+[''])[0]
print ourPath

if translatedodfpathname:
    os.system('python '+ourPath+'/xliff2odf-standalone.py "%s" "%s"'%(translatedodfpathname, Editor.currentEntryId()))

Lokalize.busyCursor(False)
