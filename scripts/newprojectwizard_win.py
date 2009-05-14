# -*- coding: utf-8 -*-
import Kross
import Lokalize
import Project
import sys,os
import codecs

def run_standalone():
    import subprocess
    
    ourPath=([p for p in sys.path if os.path.exists(p+'/newprojectwizard_win.py')]+[''])[0]
    os.system(ourPath+'/newprojectwizard.py')

    try:file=open(ourPath+'/projectconf.tmp','r')
    except: return
    projectFile=file.readline()[:-1]
    projectKind=file.readline()[:-1]
    projectName=file.readline()[:-1]
    projectSourceLang=file.readline()[:-1]
    projectTargetlang=file.readline()[:-1]
    file.close()
    os.remove(ourPath+'/projectconf.tmp')

    Project.init(projectFile, projectKind, projectName, projectSourceLang, projectTargetlang)
    Lokalize.openProject(projectFile)


run_standalone()



