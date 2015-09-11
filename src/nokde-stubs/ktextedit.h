#ifndef KTEXTEDIT_H
#define KTEXTEDIT_H

#include <QTextEdit>

class KTextEdit: public QTextEdit
{
public:
    KTextEdit(QWidget* p):QTextEdit(p){}
    void setHighlighter(void*){}

};

#endif

