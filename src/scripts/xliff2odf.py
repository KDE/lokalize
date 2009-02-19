# -*- coding: utf-8 -*-
import Kross
import Lokalize
import Project
import Editor
import sys,os
from translate.convert import xliff2odf
from translate.storage import factory

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

    store = factory.getobject(xliffpathname)
    odfpathname=path+'/'+store.getfilenames()[0]

    translatedodfpathname=os.path.splitext(odfpathname)[0]+'-'+Project.targetLangCode()+'.odt'

    xliff2odf.convertxliff(xliffinput, translatedodfpathname, odfpathname)

convert()
