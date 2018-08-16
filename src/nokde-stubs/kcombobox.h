#ifndef KCOMBOBOX_H
#define KCOMBOBOX_H

#include <QComboBox>
#include <QStringListModel>

class KComboBox: public QComboBox
{
public:
    KComboBox(QWidget* p): QComboBox(p) {}
    void setCurrentItem(const QString& s, bool insert = false)
    {
        setCurrentText(s);
        if (insert)
            if (QStringListModel* lm = qobject_cast<QStringListModel*>(model())) {
                QStringList l = lm->stringList();
                if (!l.contains(s)) l.append(s);
                lm->setStringList(l);
            }
    }
};

#endif

