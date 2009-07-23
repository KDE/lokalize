# -*- coding: utf-8 -*-
import Kross

import sys,os,codecs
ourPath=([p for p in sys.path if os.path.exists(p+'/widget-text-capture.ui')]+[''])[0]

utf8_decoder=codecs.getdecoder("utf8")

T = Kross.module("kdetranslation")
def i18n(text, args = []):
    if T is not None: return utf8_decoder(T.i18n(text, args))[0]
    # No translation module, return the untranslated string
    for a in range(len(args)): text = text.replace( ("%" + "%d" % ( a + 1 )), str(args[a]) )
    return text

forms = Kross.module("forms")
mydialog = forms.createDialog(i18n("Widget Text Capture"))
mydialog.setButtons("Ok|Cancel")
mydialog.setFaceType("Plain") #Auto Plain List Tree Tabbed
mypage = mydialog.addPage("name",i18n("Widget Text Capture"))
#mypage = mydialog.addPage("name",forms.tr("Open file"))
mywidget = forms.createWidgetFromUIFile(mypage, ourPath+"/widget-text-capture.ui")
if os.popen('kreadconfig --group Development --key CopyWidgetText').read()!='true\n':
    mywidget["none"].checked=1
elif os.popen('kreadconfig --group Development --key CopyWidgetTextCommand').read()=='\n':
    mywidget["clipboard"].checked=1
else:
    mywidget["search"].checked=1
if mydialog.exec_loop():
    if mydialog.result() == "Ok":
        if mywidget["none"].checked:
            os.system('kwriteconfig --group Development --key CopyWidgetText -type bool 0')
        else:
            os.system('kwriteconfig --group Development --key CopyWidgetText -type bool 1')
            if (mywidget["search"].checked):
                os.system("kwriteconfig --group Development --key CopyWidgetTextCommand \"/bin/sh `kde4-config --path data --locate lokalize/scripts/find-gui-text.sh` \\\"%1\\\" \\\"%2\\\"\"")
            elif mywidget["clipboard"].checked:
                os.system('kwriteconfig --group Development --key CopyWidgetTextCommand ""')


mydialog.deleteLater()