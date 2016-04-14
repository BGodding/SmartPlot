#ifndef TEXT_HELPER
#define TEXT_HELPER

#include <QTextStream>
#include <QDateTime>
#include <QMenu>
#include <QList>
#include <QFile>

class text_helper : public QObject
{
    Q_OBJECT

public:
    QString autoDetectDelimiter(QTextStream& stream);
    bool firstColumnIsIncrimental(QTextStream& stream, QString delimiter);
    QString cleanDateTimeString(QString rawDate, QString rawTime);
    void checkAndProcessColumnHeaders(QTextStream& stream, QString delimiter, QList<QVariantMap> &metaData, int firstDataColumn);
    int versionStringToInt(QString version);
    QString versionIntToString(int version);
    bool isVersionOk(QString minVersion, QString maxVersion, QString readVersion);
    int estimateLineCount( QString fileName );

    void insertDataBreak(double dateTime, QVector<QVector<double> > &seriesData, QVector<QVector<QString> > &eventData, QList<QVariantMap> &metaData);
};

#endif // TEXT_HELPER

