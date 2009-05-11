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
    if not Editor.isValid() or Editor.currentFile()=='': return

    xliffpathname=unicode(Editor.currentFile())
    (path, filename)=os.path.split(xliffpathname)
    if not filename.endswith('.xlf'): return

    store = factory.getobject(xliffpathname)
    odfpathname=store.getfilenames()[0]
    translatedodfpathname=os.path.splitext(odfpathname)[0]+'-'+Project.targetLangCode()+'.odt'
    print 'translatedodfpathname %s' % translatedodfpathname
    print 'odfpathname %s' % odfpathname
    xliffinput=XliffInput(xliffpathname,Editor.currentFileContents())

    xliff2odf.convertxliff(xliffinput, translatedodfpathname, odfpathname)

    ourpath=([p for p in sys.path if os.path.exists(p+'/xliff2odf.py')]+[''])[0]
    os.system('python "'+ourpath+'/xliff2odf-standalone.py" "%s" "%s" &'%(translatedodfpathname, Editor.currentEntryId()))

convert()

Lokalize.busyCursor(False)
