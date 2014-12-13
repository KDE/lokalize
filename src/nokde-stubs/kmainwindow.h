#ifndef KMAINWINDOW_H
#define KMAINWINDOW_H

#include <QMainWindow>

class KMainWindow: public QMainWindow
{
public:
    KMainWindow(QWidget* ):QMainWindow(0){}
    void setCaption(const QString& s, bool m=false){Q_UNUSED(m) setWindowTitle(s);}
};

#endif

