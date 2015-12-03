# -*- coding: utf-8 -*-
import os,sys
import Editor
import Project

def doCompile():
    if not Editor.isValid() or Editor.currentFile=='': return
    lang=Project.targetLangCode()

    (path, pofilename)=os.path.split(Editor.currentFile())
    (package, ext)=os.path.splitext(pofilename)
    if os.system('touch ~/.local/share/locale/%s/LC_MESSAGES' % lang)!=0:
        os.system('mkdir ~/.local/share')
        os.system('mkdir ~/.local/share/locale')
        os.system('mkdir ~/.local/share/locale/%s'  % lang)
        os.system('mkdir ~/.local/share/locale/%s/LC_MESSAGES'  % lang)

    os.system('msgfmt -o ~/.local/share/locale/%s/LC_MESSAGES/%s.mo %s' % (lang, package, Editor.currentFile()))

doCompile()
