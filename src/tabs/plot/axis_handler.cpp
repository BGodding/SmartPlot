#include "tabs/plot/axis_handler.h"

#include "utility.h"

#include "plot_interface.h"

void axis_handler::updateGraphAxes(QCustomPlot *plot)
{
    //Only rescale for the first graph
    if ( plot->plottableCount() == 1 ) {
        plot->plottable()->rescaleAxes();
    }

//    if(plot->graphCount() > 0)
//    {
//        if( plot->xAxis->ticker().dynamicCast<QCPAxisTickerDateTime>().isNull() )
//        {
//            qDebug() << "DateTime";
//        }
//        else if( qSharedPointerDynamicCast<QCPAxisTickerFixed>(plot->xAxis->ticker()).isNull() )
//        {
//            qDebug() << "Fixed";
//        }
//    }

    plot->replot();
}

//This function get pretty bloated and slow with ~20,000 events.
void axis_handler::updateAxis(QCustomPlot *plot, QList<QVariantMap> &metaData, QVector<QVector<QString> > &eventData, QMap<QString, QMap<int, QString> > &tickLabelLookup, QVector<double> &datetime,
                              QCPAxis *axis, int maxVerticalEvents)
{
    Q_UNUSED(*axis);

    if (eventData.isEmpty())
        return;

    QVector<double> eventAxisDateTimes;
    QVector<QString> eventAxisStrings;
    QMap<QString, int> eventLabelPixelWidthLookup;
    int eventLabelPixelWidth;

    double currentTickTimeStamp = std::numeric_limits<double>::quiet_NaN();
    int currentTickNumberOfRows = 0;
    QString currentTickLabel;

    QFontMetrics fm(plot->axisRect(0)->axis(QCPAxis::atTop, 0)->selectedTickLabelFont());

    int tickLabelPixelWidth = 1;
    int prevTickLabelPixelWidth = 1;
    double currentTimeStampWindow;

    //Iterate through meta data for event things and stuff
    for (int row = 0; row < eventData.size(); row++) {
        QString eventLabel;
        int searchIndex = eventData.indexOf(eventData.value(row));
        if (searchIndex >= 0 && tickLabelLookup.value(metaData.first().value("Measurement").toString()).contains(searchIndex) ) {
            eventLabel = tickLabelLookup.value(metaData.first().value("Measurement").toString()).value(searchIndex);

            if (eventLabel.isEmpty())
                continue;
        } else {
            if (isEventVisible(metaData, eventData, row, eventLabel)) {
                tickLabelLookup[metaData.first().value("Measurement").toString()][searchIndex] = eventLabel;
            } else {
                tickLabelLookup[metaData.first().value("Measurement").toString()][searchIndex] = "";
                continue;
            }
        }

        if (eventLabelPixelWidthLookup.contains(eventLabel)) {
            eventLabelPixelWidth = eventLabelPixelWidthLookup.value(eventLabel);
        } else {
            eventLabelPixelWidthLookup[eventLabel] = fm.width(eventLabel);
            eventLabelPixelWidth = eventLabelPixelWidthLookup.value(eventLabel);
        }


        //If this is true we have a new tick!
        //Give us 20% whitespace
        currentTimeStampWindow = 1.2 * (plot->xAxis->range().size() / (plot->axisRect()->width() / qMax(tickLabelPixelWidth, prevTickLabelPixelWidth)));

        //Make the assumption that the first column in the series data is the time axis
        if ( isInvalidData(currentTickTimeStamp) || ((currentTickTimeStamp + currentTimeStampWindow) <= datetime.value(row)) ) {
            //Check if we had hidden items on the previous tick before we move on
            if (currentTickNumberOfRows > maxVerticalEvents) {
                //Get the current label string
                currentTickLabel = eventAxisStrings.last();
                //Prepend the number of hidden events
                currentTickLabel.prepend(QString("(%1)").arg(currentTickNumberOfRows - maxVerticalEvents));
                //Replace the current label string
                eventAxisStrings.replace((eventAxisStrings.size() - 1), currentTickLabel);
            }
            //Add the information for the current tick
            eventAxisDateTimes.append(datetime.value(row));
            eventAxisStrings.append(eventLabel);

            currentTickTimeStamp = datetime.value(row);
            currentTickNumberOfRows = 1;
            prevTickLabelPixelWidth = tickLabelPixelWidth;
            tickLabelPixelWidth = eventLabelPixelWidth;

        } else { //Adding event to the old tick
            //Check for vertical room on current tick label
            if (!eventAxisStrings.isEmpty() && (currentTickNumberOfRows < maxVerticalEvents)) {
                currentTickLabel = eventAxisStrings.last();
                currentTickLabel.append('\n');
                currentTickLabel.append(eventLabel);
                eventAxisStrings.replace((eventAxisStrings.size() - 1), currentTickLabel);
                tickLabelPixelWidth = qMax(tickLabelPixelWidth, eventLabelPixelWidth);
            }

            currentTickNumberOfRows++;
            tickLabelPixelWidth++;
        }
    }

    plot->xAxis2->setTickLabels(true);

    //Use the x axis dates as the tick vector
    QSharedPointer<QCPAxisTickerText> ticker = plot->xAxis2->ticker().dynamicCast<QCPAxisTickerText>();
    if (ticker == nullptr) {
        //Force it to text
        plot->xAxis2->setTicker(QSharedPointer<QCPAxisTickerText>(new QCPAxisTickerText));
        ticker = plot->xAxis2->ticker().dynamicCast<QCPAxisTickerText>();
    }
    ticker->setTicks(eventAxisDateTimes, eventAxisStrings);

    plot->replot();
}

void axis_handler::updateAxis(QCustomPlot *plot, QList<QVariantMap> &metaData, const QJsonDocument &jsondoc, QCPAxis *axis, int maxVerticalEvents)
{
    Q_UNUSED(*axis);

    if (jsondoc.isEmpty())
        return;

    QVector<double> eventAxisDateTimes;
    QVector<QString> eventAxisStrings;
    double currentTickTimeStamp = std::numeric_limits<double>::quiet_NaN();
    int currentTickNumberOfRows = 0;
    QString currentTickLabel;
    QFontMetrics fm(plot->axisRect(0)->axis(QCPAxis::atTop, 0)->selectedTickLabelFont());

    int tickLabelPixelWidth = 1;
    int prevTickLabelPixelWidth = 1;
    double currentTimeStampWindow;

    double currentDataRowTimeStamp;

    QJsonObject jsonObject = jsondoc.object();
    QJsonArray resultsArray = jsonObject.value("results").toArray();

    if (resultsArray.isEmpty())
        return;

    foreach (const QJsonValue &resultsValue, resultsArray) {
        QJsonObject resultsObject = resultsValue.toObject();
        QJsonArray seriesArray = resultsObject.value("series").toArray();

        if (seriesArray.isEmpty())
            return;

        foreach (const QJsonValue &seriesValue, seriesArray) {
            QJsonObject seriesObject = seriesValue.toObject();
            QVariantMap seriesMap;

            //We might need to add another iterator level to add support for multiple events per line.....fuc
            //if(seriesObject.value("name").toString() == selectionData.value("Key Field Display Text"])
            if (seriesObject.value("name").toString() == "events") {
                //Get the column order
                QJsonArray columnArray = seriesObject.value("columns").toArray();
                QJsonArray valuesArray = seriesObject.value("values").toArray();

                int keyColumn = 0;
                int column = 0;
                foreach (const QJsonValue &columnValue, columnArray) {
                    if (columnValue.toString() == "time") {
                        keyColumn = column;
                        break;
                    }
                    column++;
                }

                foreach (const QJsonValue &valuesValue, valuesArray) {
                    //This is where we are iterating through the datas
                    //Pass valuesValue.toArray() minus the time to isEventVisible?
                    QCPGraphData dataPoint;
                    QDateTime dateTime = QDateTime::fromString(valuesValue.toArray().at(keyColumn).toString(), Qt::ISODate);
                    currentDataRowTimeStamp = dateTime.toMSecsSinceEpoch() / 1000;

                    QString eventLabel;
                    if (!isEventVisible(metaData, valuesValue.toArray(), eventLabel))
                        continue;

                    //If this is true we have a new tick!
                    //Give us 20% whitespace
                    currentTimeStampWindow = 1.2 * (plot->xAxis->range().size() / (plot->axisRect()->width() / qMax(tickLabelPixelWidth, prevTickLabelPixelWidth)));

                    //Make the assumption that the first column in the series data is the time axis
                    if ( isInvalidData(currentTickTimeStamp) || ((currentTickTimeStamp + currentTimeStampWindow) <= currentDataRowTimeStamp) ) {
                        //Check if we had hidden items on the previous tick before we move on
                        if (currentTickNumberOfRows > maxVerticalEvents) {
                            //Get the current label string
                            currentTickLabel = eventAxisStrings.last();
                            //Prepend the number of hidden events
                            currentTickLabel.prepend(QString("(%1)").arg(currentTickNumberOfRows - maxVerticalEvents));
                            //Replace the current label string
                            eventAxisStrings.replace((eventAxisStrings.size() - 1), currentTickLabel);
                        }
                        //Add the information for the current tick
                        eventAxisDateTimes.append(currentDataRowTimeStamp);
                        eventAxisStrings.append(eventLabel);

                        currentTickTimeStamp = currentDataRowTimeStamp;
                        currentTickNumberOfRows = 1;
                        prevTickLabelPixelWidth = tickLabelPixelWidth;
                        tickLabelPixelWidth = fm.size(0, eventLabel).rwidth();
                    } else { //Adding event to the old tick
                        //Check for vertical room on current tick label
                        if (!eventAxisStrings.isEmpty() && (currentTickNumberOfRows < maxVerticalEvents)) {
                            currentTickLabel = eventAxisStrings.last();
                            currentTickLabel.append('\n');
                            currentTickLabel.append(eventLabel);
                            eventAxisStrings.replace((eventAxisStrings.size() - 1), currentTickLabel);

                            if (fm.size(0, currentTickLabel).rwidth() > tickLabelPixelWidth)
                                tickLabelPixelWidth = fm.size(0, currentTickLabel).rwidth();
                        }

                        currentTickNumberOfRows++;
                    }
                }
            }
        }
    }

//    plot->xAxis2->setTickLabels(true);
//    plot->xAxis2->setAutoTicks(false);
//    plot->xAxis2->setAutoTickLabels(false);

//    //Use the x axis dates as the tick vector
//    plot->xAxis2->setTickVector(eventAxisDateTimes);
//    //Use the parsed strings vector as the labels
//    plot->xAxis2->setTickVectorLabels(eventAxisStrings);

    plot->replot();
}

void axis_handler::updateAxisTickCount(QCustomPlot *customPlot, QWidget *window)
{
    //qDebug() << "Pixel Ratio:" << devicePixelRatio() << this->width();
    //Account for higher DPI on mobile
#ifdef MOBILE

    int buttonHeight = 60 / devicePixelRatio();
    int buttonWidth = 120 / devicePixelRatio();

    customPlot->xAxis->ticker()->setTickCount(window->width()*devicePixelRatio() / 250);

    QSize screenSize = QApplication::primaryScreen()->availableSize();

    QList<QPushButton *> allPButtons = window->findChildren<QPushButton *>("+");

    if (!allPButtons.isEmpty())
        allPButtons.first()->setGeometry( (screenSize.width() - buttonWidth), (screenSize.height() - buttonHeight), buttonWidth, buttonHeight);

    allPButtons = window->findChildren<QPushButton *>("-");

    if (!allPButtons.isEmpty())
        allPButtons.first()->setGeometry( 0, (screenSize.height() - buttonHeight), buttonWidth, buttonHeight );
#else
    customPlot->xAxis->ticker()->setTickCount(window->width() / 200);
#endif
}

bool axis_handler::isEventVisible(QList<QVariantMap> &metaData, QVector<QVector<QString> > &eventData, int row, QString &tickLabel )
{
    bool visible = false;

    QVariantMap targetMap;
    targetMap["Active"] = true;

    //Iterate through all the event data sources
    for (const auto& metaDataAtIndex : metaData) {
        if ( metaDataAtIndex.value("Data Type") == "Event" ) {
            QList<QVariant> uniqueEventMetaData = metaDataAtIndex.value("Unique Event Meta Data").toList();
            //qDebug() << "uemd" << uniqueEventMetaData;

            targetMap["Key Value"] = eventData.value(row).value(metaDataAtIndex.value("Data Value Storage Index").toInt());
            targetMap["Key Field"] = metaDataAtIndex.value("Key Field");

            if (uniqueEventMetaData.contains(targetMap)) {
                if ( metaDataAtIndex.value("Action") == "OR" ) {
                    visible = true;
                }
            } else if ( metaDataAtIndex.value("Action") == "AND" ) {
                return false;
            }

            if (metaDataAtIndex.value("Tick Label") == true) {
                if (!tickLabel.isEmpty())
                    tickLabel.append("-");

                tickLabel.append(targetMap.value("Key Value").toString());
            }
        }
    }

    if (tickLabel.isEmpty())
        visible = false;

    return visible;
}

bool axis_handler::isEventVisible(QList<QVariantMap> &metaData, const QJsonArray &eventData, QString &tickLabel )
{
    bool visible = false;

    QVariantMap targetMap;
    targetMap["Active"] = true;

    for (const auto& metaDataAtIndex : metaData) {
        if ( metaDataAtIndex.value("Data Type") == "Event" ) {
            QList<QVariant> uniqueEventMetaData = metaDataAtIndex.value("Unique Event Meta Data").toList();

            targetMap["Key Value"] = eventData.at(metaDataAtIndex.value("Data Value Storage Index").toInt()).toString();

            if (uniqueEventMetaData.contains(targetMap)) {
                if ( metaDataAtIndex.value("Action") == "OR" ) {
                    visible = true;
                }
            } else if ( metaDataAtIndex.value("Action") == "AND" ) {
                return false;
            }

            if (metaDataAtIndex.value("Tick Label") == true) {
                if (!tickLabel.isEmpty())
                    tickLabel.append("-");

                tickLabel.append(targetMap.value("Key Value").toString());
            }
        }
    }

    if (tickLabel.isEmpty())
        visible = false;

    return visible;
}

bool axis_handler::isActionVisible(QVariantMap &selectionData, QList<QVariantMap> &metaData)
{
    for (const auto& metaDataAtIndex : metaData) {
        if ( (metaDataAtIndex.value("Series") == selectionData.value("Series")) &&
                (metaDataAtIndex.value("Measurement") == selectionData.value("Measurement")) ) {
            QList<QVariant> uniqueEventMetaData = metaDataAtIndex.value("Unique Event Meta Data").toList();

            //qDebug() << selectionData.value("Series") << selectionData.value("Measurement") << selectionData.value("Key Field") << selectionData.value("Key Value");

            for (const auto &uniqueEventMetaDataIndex : uniqueEventMetaData) {
                QVariantMap mData = uniqueEventMetaDataIndex.toMap();

                if ( (mData.value("Key Value") == selectionData.value("Key Value")) && (mData.value("Key Field") == selectionData.value("Key Field")) )
                    return mData.value("Active").toBool();
            }
        }
    }
    return false;
}

void axis_handler::toggleKeyValueVisibleInList(QVariantMap &selectionData, QList<QVariantMap> &metaData)
{
    if (!selectionData.contains("Key Value"))
        return;

    for (auto &keyFieldMetaData : metaData) {
        if ( (keyFieldMetaData.value("Series") == selectionData.value("Series")) &&
                (keyFieldMetaData.value("Measurement") == selectionData.value("Measurement")) ) {
            QList<QVariant> uniqueEventMetaData = keyFieldMetaData.value("Unique Event Meta Data").toList();

            for (int uniqueEventMetaDataIndex = 0; uniqueEventMetaDataIndex < uniqueEventMetaData.size(); uniqueEventMetaDataIndex++) {
                QVariantMap uniqueEventMetaDataAtIndex = uniqueEventMetaData.value(uniqueEventMetaDataIndex).toMap();
                //qDebug() << mData;

                if (selectionData.value("Key Value") == "****") {
                    uniqueEventMetaDataAtIndex["Active"] = true;
                } else if (selectionData.value("Key Value") == "    ") {
                    uniqueEventMetaDataAtIndex["Active"] = false;
                } else if ( uniqueEventMetaDataAtIndex.value("Key Value") == selectionData.value("Key Value") ) {
                    if (uniqueEventMetaDataAtIndex.value("Active") == true) {
                        uniqueEventMetaDataAtIndex["Active"] = false;
                    } else {
                        uniqueEventMetaDataAtIndex["Active"] = true;
                    }
                }
                uniqueEventMetaData[uniqueEventMetaDataIndex] = uniqueEventMetaDataAtIndex;
            }
            keyFieldMetaData["Unique Event Meta Data"] = uniqueEventMetaData;
        }
    }
}

void axis_handler::toggleKeyFieldVisibleInList(QVariantMap &selectionData, QList<QVariantMap> &metaData)
{
    if (selectionData.contains("Key Value"))
        return;

    for (auto &metaDataAtIndex : metaData) {
        if ( (metaDataAtIndex.value("Series") == selectionData.value("Series")) &&
                (metaDataAtIndex.value("Measurement") == selectionData.value("Measurement")) &&
                (metaDataAtIndex.value("Key Field") == selectionData.value("Key Field")) ) {
            if (metaDataAtIndex.value("Tick Label").toBool()) {
                metaDataAtIndex["Tick Label"] = false;
            } else {
                metaDataAtIndex["Tick Label"] = true;
            }
        }
    }
}

bool axis_handler::isAxisSelected(QCPAxis *axis)
{
    if (axis->selectedParts().testFlag(QCPAxis::spAxis) || axis->selectedParts().testFlag(QCPAxis::spTickLabels))
        return true;
    else
        return false;
}

bool axis_handler::isAxisSelected(QCustomPlot *plot, QCPAxis::AxisType AxisTypes)
{
    for (int axis = 0 ; axis < plot->selectedAxes().size() ; axis++) {
        if (plot->selectedAxes().value(axis)->axisType() == AxisTypes)
            return true;
    }
    return false;
}

bool axis_handler::isAxisSelected(QCustomPlot *plot, QCPAxis::AxisType AxisTypes, QCPAxis::SelectablePart part)
{
    for (int axis = 0 ; axis < plot->selectedAxes().size() ; axis++) {
        if ( (plot->selectedAxes().value(axis)->axisType() == AxisTypes) &&
                (plot->selectedAxes().value(axis)->selectedParts() & part))
            return true;
    }
    return false;
}

bool axis_handler::isAxisTypeSelected(QCustomPlot *customPlot, QCPAxis::AxisType type)
{
    int axisIndex = 0;
    while (axisIndex < customPlot->axisRect()->axisCount(type)) {
        if (isAxisSelected(customPlot->axisRect()->axis(type, axisIndex)) &&
                customPlot->axisRect()->axis(type, axisIndex)->axisType() == type) {
            return true;
        }
        axisIndex++;
    }
    return false;
}

void axis_handler::setAxisSelected(QCPAxis *axis)
{
    axis->setSelectedParts(QCPAxis::spAxis | QCPAxis::spTickLabels);
}

void axis_handler::setAxesSelected(const QList<QCPAxis *> &axis)
{
    int axisIndex = 0;
    while (axisIndex < axis.size()) {
        axis.at(axisIndex)->setSelectedParts(QCPAxis::spAxis | QCPAxis::spTickLabels);
        axisIndex++;
    }
}

void axis_handler::setAxesSelected(QCustomPlot *customPlot, QCPAxis::AxisType type)
{
    int axisIndex = 0;
    while (axisIndex < customPlot->axisRect()->axisCount(type)) {
        setAxisSelected(customPlot->axisRect()->axis(type, axisIndex));
        axisIndex++;
    }
}

void axis_handler::setAxisType(QCPAxis *axis, tickerType type)
{
    //Plot is current of type Date Time, switch to Numeric
    if (type == fixed && axis->ticker().dynamicCast<QCPAxisTickerFixed>().isNull()) {
        QSharedPointer<QCPAxisTickerFixed> ticker(new QCPAxisTickerFixed);
        ticker->setScaleStrategy(QCPAxisTickerFixed::ssMultiples);
        axis->setTicker(ticker);
    } else if (type == dateTime && axis->ticker().dynamicCast<QCPAxisTickerDateTime>().isNull())
        axis->setTicker(QSharedPointer<QCPAxisTickerDateTime>(new QCPAxisTickerDateTime));
}

void axis_handler::toggleAxisType(QCPAxis *axis)
{
    //Plot is current of type Date Time, switch to Numeric
    if (axis->ticker().dynamicCast<QCPAxisTickerFixed>().isNull())
        setAxisType(axis, axis_handler::fixed);
    else
        setAxisType(axis, axis_handler::dateTime);
}
