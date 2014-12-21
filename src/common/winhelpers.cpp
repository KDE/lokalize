#include <QString>
#include <windows.h>
#define SECURITY_WIN32
#include <security.h>

QString fullUserName()
{
    ushort name[100];
    unsigned long size=99;
    GetUserNameW((LPWSTR)name, &size);
    return QString::fromUtf16(name);
}
