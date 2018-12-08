#include "tabs/plot/plot_handler.h"

#include "utility.h"

#include <QJSEngine>

plot_handler::plot_handler()
{
    penColors.append(QColor( 243, 150, 69 ));   //Orange
    penColors.append(QColor(  75, 172, 198));   //Blue
    penColors.append(QColor( 128, 100, 162));   //Purple
    penColors.append(QColor( 155, 187, 89 ));   //Green
    penColors.append(QColor( 192,  80, 77 ));   //Red
    penColors.append(QColor(  31,  73, 125));   //Dark Blue
    penColors.append(QColor(  31,  31, 31 ));   //Black
}

QCPGraph *plot_handler::addPlotLine(QVector<QVector<double> > &dataVector, QVariantMap metaData)
{
    bool ok;
    int dataKeyColumn = metaData.value("Data Key Storage Index").toInt(&ok);
    int dataValueColumn = metaData.value("Data Value Storage Index").toInt(&ok);
    //qDebug() << dataKeyColumn << dataValueColumn;

    QVector<double> reducedKeyData, reducedValueData;
    int dataPoint;
    double startPoint = 0;
    double midPoint = 0;
    double endPoint = 0;
    int pointsDropped = 0;

    for (dataPoint = 0; dataPoint < dataVector.value(dataValueColumn).size(); dataPoint++) {
        //This code drops points that dont add anything to the plot. Saves up to 90%+ memory
        if ( (dataPoint > 1) && (dataPoint < dataVector.value(dataValueColumn).size() - 1)) {
            startPoint = dataVector.value(dataValueColumn).value(dataPoint - 1);
            midPoint = dataVector.value(dataValueColumn).value(dataPoint);
            endPoint = dataVector.value(dataValueColumn).value(dataPoint + 1);
            if ( (isInvalidData(startPoint) && isInvalidData(midPoint) && isInvalidData(endPoint)) ||
                    ( (startPoint == midPoint) &&  (endPoint == midPoint) ) ) {
                pointsDropped++;
                continue;
            }
        }
        reducedValueData.append(dataVector.value(dataValueColumn).value(dataPoint));
        reducedKeyData.append(dataVector.value(dataKeyColumn).value(dataPoint));
        //qDebug() << reducedKeyData.last() << reducedValueData.last() << startPoint << midPoint << endPoint << pointsDropped;
    }

    QCustomPlot *customPlot = static_cast <QCustomPlot *>(metaData["Active Plot"].value<void *>());

    qDebug() << "dropped" << pointsDropped << "of" << dataVector.value(dataValueColumn).size();

    return addPlotLine(reducedKeyData, reducedValueData, metaData.value("Key Field").toString(), customPlot);
}

QCPGraph *plot_handler::addPlotLine(QVector<double> &key, QVector<double> &value, const QString &name, QCustomPlot *plot)
{
    //Code here for graphing
    QCPGraph *newPlot = plot->addGraph();

    newPlot->setName(name);
    newPlot->setData(key, value);
    newPlot->setLineStyle(QCPGraph::lsLine);
    newPlot->setAdaptiveSampling(true);
    newPlot->setSelectable(QCP::stWhole);

    QPen graphPen;
    generatePenColor(&graphPen);
    newPlot->setPen(setPenAlpha(graphPen, 192));
    // With hardware acceleration we can now use the default selection pen
//    QCPSelectionDecorator *selectionDecorator = newPlot->selectionDecorator();
//    graphPen.setWidth(2);
//    selectionDecorator->setPen(graphPen);
//    newPlot->setSelectionDecorator(selectionDecorator);

    return newPlot;
}

QCPGraph *plot_handler::addPlotLine(QCPGraphDataContainer *dataMap, const QString &name, QCustomPlot *plot)
{
    //Code here for graphing
    QCPGraph *newPlot = plot->addGraph();

    newPlot->setName(name);
    newPlot->data()->set(*dataMap);
    newPlot->setLineStyle(QCPGraph::lsLine);
    newPlot->setAdaptiveSampling(true);
    newPlot->setSelectable(QCP::stWhole);

    QPen graphPen;
    generatePenColor(&graphPen);
    newPlot->setPen(setPenAlpha(graphPen, 192));
    // With hardware acceleration we can now use the default selection pen
//    QCPSelectionDecorator *selectionDecorator = newPlot->selectionDecorator();
//    selectionDecorator->setPen(graphPen);
//    newPlot->setSelectionDecorator(selectionDecorator);

    ah.updateGraphAxes(plot);

    plot->replot();

    return newPlot;
}

void plot_handler::plotConvert( QCPGraph *graph, const QString &functionString )
{
    QCPGraphDataContainer::iterator plotData = graph->data()->begin();

    QString scriptString("(function(x) { return ");
    scriptString.append(functionString);
    scriptString.append("; })");

    QJSEngine plotEngine;
    QJSValue plotFunction = plotEngine.evaluate(scriptString);
    QJSValueList plotValue;

    while ( plotData != graph->data()->end() ) {
        if ( !isInvalidData(plotData->value) ) {
            plotValue.clear();
            plotValue << plotData->value;
            plotData->value = plotFunction.call(plotValue).toNumber();
        }
        ++plotData;
    }
}

void plot_handler::plotAddPeriodicReport(QCPGraph *graph, QVariantMap metaData)
{
    QVector<double> x, y;
    QCPGraphDataContainer::const_iterator plotData = graph->data()->constBegin();

    QDateTime currentDataKey_qDateTime = QDateTime::fromTime_t(uint(plotData->key));

    QCustomPlot *activePlot = static_cast <QCustomPlot *>(metaData["Active Plot"].value<void *>());

    double currentDataValue = plotData->value;
    double periodStartValue = plotData->value;
    double periodBegin = plotData->key;
    double periodEnd;
    double periodInterval;

    QDateTime periodEnd_qDateTime;

    bool moreData = true;

    if (metaData.value("Interval Value").toInt() == 0)
        return;

    //Line up start date time to nearest interval
    if ( metaData.value("Interval Type") == "Count" ) {
        periodBegin -= int(periodBegin) % metaData["Interval Value"].toInt();
    } else {
        while (true) {
            if (metaData.value("Interval Type") == "Second")
                break;

            periodBegin -= currentDataKey_qDateTime.time().second();

            if (metaData.value("Interval Type") == "Minute")
                break;

            periodBegin -= currentDataKey_qDateTime.time().minute() * 60;

            if (metaData.value("Interval Type") == "Hour")
                break;

            periodBegin -= currentDataKey_qDateTime.time().hour() * 3600;

            if (metaData.value("Interval Type") == "Day")
                break;

            if (metaData.value("Interval Type") == "Week")
                periodBegin -= (currentDataKey_qDateTime.date().dayOfWeek() - 1) * 86400;
            else if (metaData.value("Interval Type") == "Month")
                periodBegin -= (currentDataKey_qDateTime.date().day() - 1) * 86400;
            else if (metaData.value("Interval Type") == "Year")
                periodBegin -= (currentDataKey_qDateTime.date().dayOfYear () - 1) * 86400;

            break;
        }
    }

    periodEnd_qDateTime = QDateTime::fromTime_t(uint(periodBegin));

    if ( metaData.value("Interval Type") ==  "Count" ) {
        periodEnd = periodBegin + metaData.value("Interval Value").toInt();
    } else {
        IncrementDateTime( metaData, &periodEnd_qDateTime );
        periodEnd = periodEnd_qDateTime.toTime_t();
    }

    while (moreData) {
        //qDebug() << periodBegin << periodEnd << plotData.value().key << plotData.value().value;
        if (plotData == graph->data()->constEnd()) {
            moreData = false;
        } else {
            //Check for NaN
            if (isInvalidData(plotData->value)) {
                ++plotData;
                continue;
            }
            if (plotData->key < periodEnd) {
                currentDataValue = plotData->value;
                ++plotData;
                continue;
            }
        }

        //Found our next applicable point
        if ( (plotData->key >= periodEnd) || !moreData) {
            periodInterval = periodEnd - periodBegin;

            x.append( (periodEnd - (periodInterval / 2)) );
            y.append(currentDataValue - periodStartValue);

            periodBegin = periodEnd;

            if ( metaData.value("Interval Type") ==  "Count" ) {
                periodEnd += metaData.value("Interval Value").toInt();
            } else {
                IncrementDateTime( metaData, &periodEnd_qDateTime );
                periodEnd = periodEnd_qDateTime.toTime_t();
            }

            periodStartValue = currentDataValue;
        }
    }

    if (x.isEmpty() || y.isEmpty())
        return;

    auto *periodic = new QCPBars(activePlot->xAxis, activePlot->yAxis);
    periodic->setName(graph->name());
    periodic->setSelectable(QCP::stWhole);

    if ( (metaData.value("Interval Type") == "Year") || (metaData.value("Interval Type") == "Month") ) {
        //Intervals that vary in length can't use the full width
        periodic->setWidth(periodInterval * .75);
    } else
        periodic->setWidth(periodInterval);

    periodic->setData(x, y);
}

void plot_handler::generatePenColor(QPen *pen)
{
    static int i = 0;
    pen->setColor(penColors.value(i));
    i++;
    if (i >= penColors.size())
        i = 0;
}

//QCPGraph *smart_plot::addPlotLine(QCPDataMap *data, QString *name)
//{
//    Q_UNUSED(*name);
//    //Code here for graphing
//    QCPGraph *newPlot = activePlot()->addGraph();
//    //activePlot()->graph()->setName(name);
//    activePlot()->graph()->setData(data);
//    activePlot()->graph()->setLineStyle(QCPGraph::lsLine);
//    activePlot()->graph()->setAdaptiveSampling(true);

//    QPen graphPen;
//    plotInterface.generatePenColor(&graphPen);
//    activePlot()->graph()->setPen(plotInterface.setPenAlpha(graphPen, 192));
//    activePlot()->graph()->setSelectedPen(graphPen);

//    plotInterface.updateGraphAxes(activePlot());
//    return newPlot;
//}

//QCPBars *smart_plot::addPlotBar(QVector<QVector<double> > *dataVector, QVector<QString> *headers, int dataColumn)
//{
//    Q_UNUSED(*dataVector);
//    Q_UNUSED(*headers);
//    Q_UNUSED(dataColumn);
//    QCPBars *newBar = new QCPBars(activePlot()->xAxis, activePlot()->yAxis);

//    activePlot()->addPlottable(bar);
//    bar->setName(tr("Removed"));
//    bar->setData(dates, removedLines);
//    bar->setBrush(Qt::darkGray);
//    bar->setWidth(resolution);
//    return newBar;
//}
