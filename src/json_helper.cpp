#include "json_helper.h"

#include <QDebug>

void json_helper::jsonDumpObject(const QJsonObject &jsonObject)
{
    qDebug() << "Object Keys: " << jsonObject.keys();
    QStringList keyList = jsonObject.keys();
    int key = 0;

    for (; key < keyList.size(); key++) {
        QJsonValue value = jsonObject.value(keyList.value(key));
        if (value.isArray()) {
            QJsonArray valueArray = value.toArray();
            jsonDumpArray(valueArray);
        }
        if (value.isObject()) {
            QJsonObject valuesObject = value.toObject();
            jsonDumpObject(valuesObject);
        }
        if (value.isDouble()) {
            qDebug() << keyList.value(key) << value.toDouble();
        }
        if (value.isString()) {
            qDebug() << keyList.value(key) << value.toString();
        }
    }
}

void json_helper::jsonDumpArray(QJsonArray jsonArray)
{
    foreach (const QJsonValue &value, jsonArray) {
        if (value.isArray()) {
            QJsonArray valueArray = value.toArray();
            jsonDumpArray(valueArray);

        }
        if (value.isObject()) {
            QJsonObject valuesObject = value.toObject();
            jsonDumpObject(valuesObject);
        }
        if (value.isDouble()) {
            qDebug() << value.toDouble();
        }
        if (value.isString()) {
            qDebug() << value.toString();
        }
    }
}

void json_helper::jsonDumpDoc(const QJsonDocument &jsondoc)
{
    if (jsondoc.isArray()) {
        QJsonArray jsonArray = jsondoc.array();
        qDebug() << "Array: " << jsonArray;
        jsonDumpArray(jsonArray);
    }
    if (jsondoc.isObject()) {
        QJsonObject jsonObject = jsondoc.object();
        qDebug() << "Object: " << jsonObject;
        jsonDumpObject(jsonObject);
    }
}

void json_helper::getValues(const QJsonDocument &jsondoc, QVector<double> &key, QVector<double> &value)
{
    QJsonObject jsonObject = jsondoc.object();
    QJsonArray resultsArray = jsonObject["results"].toArray();

    if (resultsArray.isEmpty())
        return;

    foreach (const QJsonValue &resultsValue, resultsArray) {
        QJsonObject resultsObject = resultsValue.toObject();
        QJsonArray seriesArray = resultsObject["series"].toArray();

        foreach (const QJsonValue &seriesValue, seriesArray) {
            QJsonObject seriesObject = seriesValue.toObject();
            QJsonArray valuesArray = seriesObject["values"].toArray();

            foreach (const QJsonValue &valuesValue, valuesArray) {
                QJsonArray subValuesArray = valuesValue.toArray();
                foreach (const QJsonValue &subValuesValue, subValuesArray) {
                    if (subValuesValue.isDouble()) {
                        if (subValuesValue.toDouble() > 946688461)
                            key.append(subValuesValue.toDouble());
                        else
                            value.append(subValuesValue.toDouble());
                    }
                }
                //qDebug() << propertyInfo;
            }
        }
    }
}

//QList<QVariant > json_helper::getValues(QJsonDocument jsondoc, QVector<double> &key, QVector<double> &value)
//{

//}

QList<QVariant > json_helper::getValues(const QJsonDocument &jsondoc)
{
    QList<QVariant > values;
    QMap<QString, QVariant> propertyInfo;

    int position = 0;

    QJsonObject jsonObject = jsondoc.object();
    QJsonArray resultsArray = jsonObject["results"].toArray();

    if (resultsArray.isEmpty())
        return values;

    foreach (const QJsonValue &value, resultsArray) {
        QJsonObject resultsObject = value.toObject();
        QJsonArray seriesArray = resultsObject["series"].toArray();

        foreach (const QJsonValue &seriesValue, seriesArray) {
            QJsonObject seriesObject = seriesValue.toObject();
            QJsonArray columnArray = seriesObject["columns"].toArray();
            QJsonArray valuesArray = seriesObject["values"].toArray();

            foreach (const QJsonValue &valuesValue, valuesArray) {
                position = 0;
                propertyInfo.clear();
                propertyInfo.insert("name", seriesObject["name"].toString());

                QJsonArray subValuesArray = valuesValue.toArray();
                foreach (const QJsonValue &subValuesValue, subValuesArray) {
                    if (subValuesValue.isString()) {
                        propertyInfo.insert(columnArray.at(position).toString(), subValuesValue.toString());
                        values.append(subValuesValue.toString());
                    } else if (subValuesValue.isDouble()) {
                        propertyInfo.insert(columnArray.at(position).toString(), subValuesValue.toDouble());
                        values.append(subValuesValue.toDouble());
                    }
                    position++;
                }
                //qDebug() << propertyInfo;
            }
        }
    }

    return values;
}
