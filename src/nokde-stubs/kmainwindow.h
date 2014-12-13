#ifndef KMAINWINDOW_H
#define KMAINWINDOW_H

#include <QMainWindow>

class KMainWindow: public QMainWindow
{
public:
    KMainWindow(QWidget* p):QMainWindow(p){}
    void setCaption(const QString& s, bool m=false){setWindowTitle(s);}
};

#endif

