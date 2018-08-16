#ifndef KMAINWINDOW_H
#define KMAINWINDOW_H

#include "lokalize_debug.h"

#include <QMainWindow>
#include <QCloseEvent>
#include <QApplication>

class KMainWindow: public QMainWindow
{
public:
    KMainWindow(QWidget*): QMainWindow(0)
    {
        setAttribute(Qt::WA_DeleteOnClose, true);
    }
    void setCaption(const QString& s, bool m = false)
    {
        Q_UNUSED(m) setWindowTitle(s);
    }

    virtual bool queryClose()
    {
        return true;
    }

protected:
    void closeEvent(QCloseEvent *event)
    {
        event->setAccepted(queryClose());
    }
};

#endif

