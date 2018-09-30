#include "utility.h"

void generateUniqueLists( QVector<QVector<QString> > *data, QList<QVariantMap> &metaData)
{
    for(int measurementMetaDataIndex = 0; measurementMetaDataIndex < metaData.size(); measurementMetaDataIndex++)
    {
        QVariantMap& keyFieldMetaData = metaData[measurementMetaDataIndex];

        if( (keyFieldMetaData.value("Data Type") == "Event") )
        {
            QList<QString> list;
            QList<QVariant> uniqueEventMetaData;
            QVariantMap metaData;
            int column = keyFieldMetaData.value("Data Value Storage Index").toInt();

            generateUniqueList(data, column, list);
            metaData.clear();

            metaData["Key Field"] = keyFieldMetaData.value("Key Field");

            for(int row = 0; row < list.size(); row++)
            {
                metaData["Key Value"] = list.value(row);
                metaData["Active"] = false;
                uniqueEventMetaData.append(metaData);
            }
            keyFieldMetaData["Unique Event Meta Data"] = uniqueEventMetaData;
        }
    }
}

void generateUniqueList(const QVector<QString> &data, QList<QString> &list)
{
    int row;
    list.clear();

    for(row = 0; row < data.size(); row++)
    {
        if(list.indexOf(data.value(row)) == -1)
            list.append(data.value(row));
    }
    qSort(list);
}

void generateUniqueList(QVector<QVector<QString> > *data, int column, QList<QString> &list)
{
    int row;
    list.clear();

    for(row = 0; row < data->size(); row++)
    {
        if(list.indexOf(data->value(row).value(column)) == -1)
            list.append(data->value(row).value(column));
    }
    qSort(list);
}

QPen setPenAlpha(QPen pen, int alpha)
{
    QColor tempColor = pen.color();
    tempColor.setAlpha(alpha);
    pen.setColor(tempColor);
    return pen;
}

void IncrementDateTime( QVariantMap metaData, QDateTime *dateTime )
{
    if( metaData.value("Interval Type") == "Year" )
        *dateTime = dateTime->addYears(metaData.value("Interval Value").toInt());
    else if( metaData.value("Interval Type") == "Month" )
        *dateTime = dateTime->addMonths(metaData.value("Interval Value").toInt());
    else if( metaData.value("Interval Type") == "Week" )
        *dateTime = dateTime->addDays(7*metaData.value("Interval Value").toInt());
    else if( metaData.value("Interval Type") == "Day" )
        *dateTime = dateTime->addDays(metaData.value("Interval Value").toInt());
    else if( metaData.value("Interval Type") == "Hour" )
        *dateTime = dateTime->addSecs(3600*metaData.value("Interval Value").toInt());
    else if( metaData.value("Interval Type") == "Minute" )
        *dateTime = dateTime->addSecs(60*metaData.value("Interval Value").toInt());
    else if( metaData.value("Interval Type") == "Second" )
        *dateTime = dateTime->addSecs(metaData.value("Interval Value").toInt());
}

void scaleMenuForScreen(QMenu* menu, QCustomPlot* plot)
{
//    #if MOBILE
    QFont newFont("Monospace");
    while( (menu->sizeHint().width() > (plot->width()*.9)) && (menu->fontInfo().pointSize() > 6) )
    {
        newFont.setPointSize(newFont.pointSize()-1);
        menu->setFont(newFont);
    }
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

    if((hours == 0)&&(days == 0))
        return res.sprintf("%02d:%02d", minutes, seconds);
    else if (days == 0)
        return res.sprintf("%02d:%02d:%02d", hours, minutes, seconds);
    else
        return res.sprintf("%dd%02d:%02d:%02d", days, hours, minutes, seconds);
}
