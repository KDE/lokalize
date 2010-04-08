# -*- coding: utf-8 -*-
import Kross,Lokalize,Project,Editor
import sys,os,time,datetime,re,string,codecs

try:
    from translate.storage import factory
    import subprocess
    ourPath=([p for p in sys.path if os.path.exists(p+'/msgmerge.py')]+[''])[0]
    tt_present=True
except:
    tt_present=False

utf8_decoder=codecs.getdecoder("utf8")
T = Kross.module("kdetranslation")
def i18n(text, args = []):
    if T is not None: return utf8_decoder(T.i18n(text, args))[0]
    # No translation module, return the untranslated string
    for a in range(len(args)): text = text.replace( ("%" + "%d" % ( a + 1 )), str(args[a]) )
    return text



def merge():
    if Lokalize.projectOverview(): files=Lokalize.projectOverview().selectedItems()
    elif Editor.isValid():         files=[Editor.currentFile()]

    forms = Kross.module("forms")

    if not files: return
    if files[0].endswith('.po'):
        mergeOne=mergeOneGettext
        if os.system('which msgmerge')!=0:
            forms.showMessageBox("Error", i18n("Gettext not found"), i18n("Install gettext package for this feature to work"))
    else:
        if not tt_present:
            print 'error'
            #forms = Kross.module("forms")
            #forms.showMessageBox("Error", i18n("Translate-tolkit not found"), i18n("Install translate-toolkit package for this feature to work"))
        mergeOne=mergeOneOdf

    okCount=0
    progress=0
    if len(files)>1:
        progress=forms.showProgressDialog(i18n("Updating from templates..."), "")
        progress.setRange(0,len(files))
        #progress.setMaximum(len(files))
        counter=0

    for po in files:
        if progress:
            progress.addText(po)
            progress.setValue(counter)
            counter+=1

        ok=mergeOne(po)
        okCount+=ok
        if ok:
            editor=Lokalize.editorForFile(po)
            if editor:
                editor.reloadFile()

    if progress:
        progress.deleteLater()
    if mergeOne==mergeOneGettext:
        if len(files)==1:
            pot=potForPo(files[0])
            if okCount:
                potModifSeconds=os.path.getmtime(pot)
                potModifDelta=datetime.timedelta(seconds=time.time()-potModifSeconds)
                potModifStr=time.strftime('%X %x %Z', time.localtime(potModifSeconds))
                forms.showMessageBox("Information", i18n("Merge has been completed"), i18n("Merge has been completed.\nTemplate modification time: %1 (%2 days ago).",[str(potModifStr),potModifDelta.days]))
            else:
                if not os.path.exists(pot):
                    forms.showMessageBox("Error", i18n("Merge failed."), i18n("Could not find template file for the merge:\n%1",[pot]))

def potForPo(po):
    (path, pofilename)=os.path.split(po)
    path=path.replace(Project.translationsRoot(),Project.templatesRoot())

    (package, ext)=os.path.splitext(pofilename)
    return path+'/'+package+'.pot'
    

def mergeOneGettext(po):
    if po=='' or not po.endswith('.po'): return False
    if Project.translationsRoot() not in po: return False

    pot=potForPo(po)

    pomtime=os.path.getmtime(po)
    os.system('msgmerge --previous -U %s %s' % (po,pot))
    return not pomtime==os.path.getmtime(po)

def mergeOneOdf(xliffpathname):
    xliffpathname=Editor.currentFile()
    (path, filename)=os.path.split(xliffpathname)
    if not filename.endswith('.xlf'): return
    xlifftemplatepathname=path+'/t_'+filename
    print xlifftemplatepathname

    store = factory.getobject(xliffpathname)
    odfpathname=store.getfilenames()[0]
    
    if odfpathname.startswith('NoName'):
        print 'translate-toolkit is too old'
        odfpathname=os.path.splitext(xliffpathname)[0]+'.odt'
    if not os.path.exists(odfpathname): return

    print 'odf2xliff via subprocess.call', unicode(odfpathname),  unicode(xlifftemplatepathname)
    try:
        retcode = subprocess.call(['odf2xliff', unicode(odfpathname),  unicode(xlifftemplatepathname)])
        print >>sys.stderr
    except OSError, e:
        print >>sys.stderr, "Execution failed:", e

    cmd='%s/odf/xliffmerge.py -i %s -t %s -o %s' % (ourPath,xliffpathname,xlifftemplatepathname,xliffpathname)
    if os.name!='nt': cmd='python '+cmd
    else: cmd=cmd.replace('/','\\')
    os.system(cmd)

    


merge()
