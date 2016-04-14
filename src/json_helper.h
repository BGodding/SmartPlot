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
    void jsonDumpObject(QJsonObject jsonObject);
    void jsonDumpArray(QJsonArray jsonArray);
    void jsonDumpDoc(QJsonDocument jsondoc);
    void getValues(QJsonDocument jsondoc, QVector<double> &key, QVector<double> &value);
    QList<QVariant > getValues(QJsonDocument jsondoc);

};

#endif // JSON_HELPER
