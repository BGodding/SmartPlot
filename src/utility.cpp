#include "utility.h"

void generateUniqueLists( QVector<QVector<QString> > *data, QList<QVariantMap> &metaData)
{
    for (auto &keyFieldMetaData : metaData) {
        if ( keyFieldMetaData.value("Data Type") == "Event" ) {
            QList<QString> list;
            QList<QVariant> uniqueEventMetaData;
            QVariantMap entryMetaData;
            int column = keyFieldMetaData.value("Data Value Storage Index").toInt();

            generateUniqueList(data, column, list);
            entryMetaData.clear();

            entryMetaData["Key Field"] = keyFieldMetaData.value("Key Field");

            for (int row = 0; row < list.size(); row++) {
                entryMetaData["Key Value"] = list.value(row);
                entryMetaData["Active"] = false;
                uniqueEventMetaData.append(entryMetaData);
            }
            keyFieldMetaData["Unique Event Meta Data"] = uniqueEventMetaData;
        }
    }
}

// referencePairs uses the key as the primary data source and the value as the reference data source
// It is also to  be noted the reference data source will no longer be part of the unique list
void generateUniqueLists( QVector<QVector<QString> > *data, QList<QVariantMap> &metaData,
                          const QMap<int, int> &referencePairs)
{
    for (int metaDataIndex = 0; metaDataIndex < metaData.size(); metaDataIndex++) {
        //Check if the data source is being used as a reference. Skip if it is.
//        if( referencePairs.key(measurementMetaDataIndex, std::numeric_limits<int>::max()) != std::numeric_limits<int>::max() )
//        {
//            qDebug() << "Skipping" << measurementMetaDataIndex;
//            continue;
//        }

        QVariantMap &metaDataAtIndex = metaData[metaDataIndex];

        if ( metaDataAtIndex.value("Data Type") == "Event" ) {
            QList<QVariant> uniqueDataMapList;
            QVariantMap uniqueDataMap;
            QVariantMap keyValueDisplayStringMap;
            int primaryColumn = metaDataAtIndex.value("Data Value Storage Index").toInt();

            uniqueDataMap.clear();
            uniqueDataMap["Key Field"] = metaDataAtIndex.value("Key Field");
            uniqueDataMap["Active"] = false;

            //Check if the data source has a reference source
            if (referencePairs.contains(metaDataIndex)) {
                QVariantMap &metaDataAtReferenceIndex = metaData[referencePairs.value(metaDataIndex)];
                int referenceColumn = metaDataAtReferenceIndex.value("Data Value Storage Index").toInt();
                QMap<QString, QString> map = generateUniqueListWithRef(data, primaryColumn, referenceColumn);
                for (const auto &e : map.keys()) {
                    uniqueDataMap["Key Value"] = e;
                    uniqueDataMapList.append(uniqueDataMap);
                    keyValueDisplayStringMap.insert(QString(uniqueDataMap["Key Field"].toString() +
                                                            uniqueDataMap["Key Value"].toString()),
                                                    QString(uniqueDataMap["Key Value"].toString() + " - " + map.value(e)));
                }
            } else {
                QList<QString> list;
                generateUniqueList(data, primaryColumn, list);
                for (int row = 0; row < list.size(); row++) {
                    uniqueDataMap["Key Value"] = list.value(row);
                    uniqueDataMapList.append(uniqueDataMap);
                    keyValueDisplayStringMap.insert(QString(uniqueDataMap["Key Field"].toString() +
                                                            uniqueDataMap["Key Value"].toString()),
                                                    list.value(row));
                }
            }
            metaDataAtIndex["Unique Event Meta Data"] = uniqueDataMapList;
            metaDataAtIndex["Key Value Display String"] = keyValueDisplayStringMap;
        }
    }
}

void generateUniqueList(const QVector<QString> &data, QList<QString> &list)
{
    int row;
    list.clear();

    for (row = 0; row < data.size(); row++) {
        if (list.indexOf(data.value(row)) == -1)
            list.append(data.value(row));
    }
    qSort(list);
}

void generateUniqueList(QVector<QVector<QString> > *data, int column, QList<QString> &list)
{
    int row;
    list.clear();

    for (row = 0; row < data->size(); row++) {
        if (list.indexOf(data->value(row).value(column)) == -1)
            list.append(data->value(row).value(column));
    }
    qSort(list);
}

QMap<QString, QString> generateUniqueListWithRef(QVector<QVector<QString> > *data, int pColumn,
                                                 int rColumn)
{
    int row;
    QMap<QString, QString> uniqueData;

    for (row = 0; row < data->size(); row++) {
        if (!uniqueData.contains(data->value(row).value(pColumn)))
            uniqueData[data->value(row).value(pColumn)] = data->value(row).value(rColumn);
    }
    return uniqueData;
}

QPen setPenAlpha(QPen pen, int alpha)
{
    QColor tempColor = pen.color();
    tempColor.setAlpha(alpha);
    pen.setColor(tempColor);
    return pen;
}

void IncrementDateTime( const QVariantMap &metaData, QDateTime *dateTime )
{
    if ( metaData.value("Interval Type") == "Year" )
        *dateTime = dateTime->addYears(metaData.value("Interval Value").toInt());
    else if ( metaData.value("Interval Type") == "Month" )
        *dateTime = dateTime->addMonths(metaData.value("Interval Value").toInt());
    else if ( metaData.value("Interval Type") == "Week" )
        *dateTime = dateTime->addDays(7 * metaData.value("Interval Value").toInt());
    else if ( metaData.value("Interval Type") == "Day" )
        *dateTime = dateTime->addDays(metaData.value("Interval Value").toInt());
    else if ( metaData.value("Interval Type") == "Hour" )
        *dateTime = dateTime->addSecs(3600 * metaData.value("Interval Value").toInt());
    else if ( metaData.value("Interval Type") == "Minute" )
        *dateTime = dateTime->addSecs(60 * metaData.value("Interval Value").toInt());
    else if ( metaData.value("Interval Type") == "Second" )
        *dateTime = dateTime->addSecs(metaData.value("Interval Value").toInt());
}

void scaleMenuForScreen(QMenu *menu, QCustomPlot *plot)
{
//    #if MOBILE
    QFont newFont("Monospace");
    while ( (menu->sizeHint().width() > (plot->width()*.9)) && (menu->fontInfo().pointSize() > 6) ) {
        newFont.setPointSize(newFont.pointSize() - 1);
        menu->setFont(newFont);
    }
    //menu->setStyleSheet(" QMenu::item { padding: 1px 1px 1px 18px; border: 1px solid transparent; }");
//    #endif
    menu->setWindowOpacity(.90);
}

QString seconds_to_DHMS(quint32 duration)
{
    QString res;

    int seconds = int(duration % 60);
    duration /= 60;
    int minutes = int(duration % 60);
    duration /= 60;
    int hours = int(duration % 24);
    int days = int(duration / 24);

    if ((hours == 0) && (days == 0))
        return res.sprintf("%02d:%02d", minutes, seconds);
    if (days == 0)
        return res.sprintf("%02d:%02d:%02d", hours, minutes, seconds);
    else
        return res.sprintf("%dd%02d:%02d:%02d", days, hours, minutes, seconds);
}
