#include <QString>
#import <Foundation/Foundation.h>

QString fullUserName()
{
    return QString::fromNSString(NSFullUserName());
}