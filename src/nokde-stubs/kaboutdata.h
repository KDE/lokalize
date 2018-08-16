#include <QString>
#include <QObject>
#include <QVector>

namespace KAboutLicense
{
enum L {GPL};
};
struct Credit {
    QString name, what, mail, site;
};
class KAboutData: public QObject
{
    Q_OBJECT
public:
    KAboutData(const QString&, const QString& n, const QString& v, const QString& d, KAboutLicense::L, const QString& c);
    void addAuthor(const QString& name, const QString&, const QString& mail);
    void addCredit(const QString& who, const QString& forwhat, const QString& mail, const QString& site = QString());

    static KAboutData* instance;
public slots:
    void doAbout();
private:
    QString name, version, description, copyright;
    QVector<Credit> credits;
};