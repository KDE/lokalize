# -*- coding: utf-8 -*-
import Kross
import Lokalize
import Project
import sys,os
from translate.convert import xliff2odf

def convert():
    if not Lokalize.activeEditor() or Lokalize.activeEditor().currentFile()=='': return

    
    xliffpathname=Lokalize.activeEditor().currentFile()
    (path, filename)=xliffpathname
    if not filename.endswith('.xlf'): return
    odfpathname=os.path.splitext(filename)[0]+'.odt'
    translatedodfpathname=os.path.splitext(filename)[0]+'-'+Project.targetLangCode()+'.odt'

    print xliffpathname
    print odfpathname
    print translatedodfpathname
    xliff2odf.main(args(xliffpathname, translatedodfpathname, t=odfpathname))

convert()
