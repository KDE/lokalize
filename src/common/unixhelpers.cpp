#include <QString>
#include <unistd.h>

QString fullUserName()
{
    return QString::fromUtf8(getlogin());
}
