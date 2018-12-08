#ifndef TEXT_HELPER
#define TEXT_HELPER

#include <QObject>
#include <QTextStream>
#include <QDateTime>
#include <QMenu>
#include <QList>
#include <QFile>

class text_helper : public QObject
{
    Q_OBJECT

public:
    QString autoDetectDelimiter(QTextStream &stream);
    bool firstColumnIsIncrimental(QTextStream &stream, const QString &delimiter);
    QString cleanDateTimeString(const QString &rawDate, const QString &rawTime);
    void checkAndProcessColumnHeaders(QTextStream &stream, const QString &delimiter, QList<QVariantMap> &metaData, int firstDataColumn);
    int versionStringToInt(QString version);
    QString versionIntToString(int version);
    bool isVersionOk(QString minVersion, QString maxVersion, const QString &readVersion);
    int estimateLineCount( const QString &fileName );

    void insertDataBreak(double dateTime, QVector<QVector<double> > &seriesData, QVector<QVector<QString> > &eventData, QList<QVariantMap> &metaData);
};

#endif // TEXT_HELPER

