import Kross

import sys,os
ourPath=''
for p in sys.path:
    if os.path.exists(p+'/widget-text-capture.ui'):
        ourPath=p
        break



forms = Kross.module("forms")
mydialog = forms.createDialog("Widget Text Capture")
mydialog.setButtons("Ok|Cancel")
mydialog.setFaceType("Plain") #Auto Plain List Tree Tabbed
mypage = mydialog.addPage("name","Widget Text Capture")
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
                os.system("kwriteconfig --group Development --key CopyWidgetTextCommand \"`kde4-config --path data --locate lokalize/find-gui-text.sh` \\\"%1\\\" \\\"%2\\\"\"")
                os.system('chmod a+x `kde4-config --path data --locate lokalize/find-gui-text.sh`')
            elif mywidget["clipboard"].checked:
                os.system('kwriteconfig --group Development --key CopyWidgetTextCommand ""')


mydialog.deleteLater()