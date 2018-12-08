#ifndef JSON_HELPER
#define JSON_HELPER

#include <QObject>

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

class json_helper : public QObject
{
    Q_OBJECT

public:
    void jsonDumpObject(const QJsonObject &jsonObject);
    void jsonDumpArray(QJsonArray jsonArray);
    void jsonDumpDoc(const QJsonDocument &jsondoc);
    void getValues(const QJsonDocument &jsondoc, QVector<double> &key, QVector<double> &value);
    QList<QVariant > getValues(const QJsonDocument &jsondoc);

};

#endif // JSON_HELPER
