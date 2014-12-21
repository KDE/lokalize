#include "klocalizedstring.h"

QString i18nc(const char* y, const char* x){return QObject::tr(x, y);}
QString i18nc(const char* y, const char* x, int n){return QObject::tr(x, y, n);}
QString i18nc(const char* y, const char* x, const QString& s){return QObject::tr(x, y).arg(s);}
QString i18nc(const char* y, const char* x, const QString& s1, const QString& s2){return QObject::tr(x, y).arg(s1).arg(s2);}
QString i18nc(const char* y, const char* x, int n, int m){return QObject::tr(x, y).arg(n).arg(m);}
QString i18n(const char* x, int n, int m){return QObject::tr(x).arg(n).arg(m);}
QString i18n(const char* x, const QString& s1, const QString& s2){return QObject::tr(x).arg(s1).arg(s2);}
QString i18n(const char* x){return QObject::tr(x);}
