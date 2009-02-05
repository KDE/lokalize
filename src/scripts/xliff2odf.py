# -*- coding: utf-8 -*-
import Kross
import Lokalize
import Project
import sys,os
from translate.convert import xliff2odf

def convert():
    def args(src, tgt, **kwargs):
        arg_list = [u'--errorlevel=traceback', src, tgt]
        for flag, value in kwargs.iteritems():
            value = unicode(value)
            if len(flag) == 1:
                arg_list.append(u'-%s' % flag)
            else:
                arg_list.append(u'--%s' % flag)
            if value is not None:
                arg_list.append(value)
        return arg_list

    if not Lokalize.activeEditor() or Lokalize.activeEditor().currentFile()=='': return
    
    xliffpathname=Lokalize.activeEditor().currentFile()
    (path, filename)=os.path.split(xliffpathname)
    if not filename.endswith('.xlf'): return
    odfpathname=os.path.splitext(xliffpathname)[0]+'.odt'
    translatedodfpathname=os.path.splitext(xliffpathname)[0]+'-'+Project.targetLangCode()+'.odt'

    print xliffpathname
    print odfpathname
    print translatedodfpathname
    xliff2odf.main(args(xliffpathname, translatedodfpathname, t=odfpathname))

convert()
