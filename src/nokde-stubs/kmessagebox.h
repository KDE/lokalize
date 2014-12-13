#ifndef KMESSAGEBOX_H
#define KMESSAGEBOX_H

#include <QMessageBox>

namespace KStandardGuiItem
{
    static inline int save(){return 0;}
    static inline int discard(){return 0;}
};

class KMessageBox: public QMessageBox
{
public:
    enum {Continue=QMessageBox::Ignore};
    static QMessageBox::StandardButton warningYesNoCancel(QWidget *parent, const QString &text,
                              const QString &caption,
                              int y=0,
                              int n=0,
                              int c=0,
                              const QString &dontAskAgainName=QString())
    {
        Q_UNUSED(y) Q_UNUSED(n) Q_UNUSED(c) Q_UNUSED(dontAskAgainName)
        return warning(parent, caption, text, Yes|No|Cancel, Yes);
    }
    static QMessageBox::StandardButton questionYesNo(QWidget *parent, const QString &text,
                              const QString &caption,
                              int y=0,
                              int n=0,
                              const QString &dontAskAgainName=QString())
    {
        Q_UNUSED(y) Q_UNUSED(n) Q_UNUSED(dontAskAgainName)
        return question(parent, caption, text, Yes|No|Cancel, Yes);
    }
    static void information(QWidget *parent,
                     const QString &text,
                     const QString &caption = QString(),
                     const QString &dontShowAgainName = QString())
    {
        Q_UNUSED(dontShowAgainName)
        QMessageBox::information(parent, caption, text);
    }

    static QMessageBox::StandardButton error(QWidget *parent, const QString &text)
    {
        return critical(parent, QString(), text);
    }
};

#endif

