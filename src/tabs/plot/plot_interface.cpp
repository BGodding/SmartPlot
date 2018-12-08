#include "plot_interface.h"

#include "utility.h"
#include "tabs/plot/plot_analytics.h"

#include <QTranslator>
#include <QJSEngine>

//TODO: If a given axis only correlates to a single plottable change its color to match that plot
void plot_interface::selectionChanged(QCustomPlot *customPlot)
{
    //Reset range and drag ranges
    customPlot->axisRect()->setRangeZoom(Qt::Horizontal | Qt::Vertical);
    customPlot->axisRect()->setRangeDrag(Qt::Horizontal | Qt::Vertical);

    /*
    normally, axis base line, axis tick labels and axis labels are selectable separately, but we want
    the user only to be able to select the axis as a whole, so we tie the selected states of the tick labels
    and the axis base line together. However, the axis label shall be selectable individually.

    The selection state of the bottom and top axes shall be synchronized.

    Further, we want to synchronize the selection of the graphs with the selection state of the respective
    legend item belonging to that graph. So the user can select a graph by either clicking on the graph itself
    or on its legend item.
    */

    // make top and bottom axes be selected synchronously, and handle axis and tick labels as one selectable object:
    if ( ah.isAxisTypeSelected(customPlot, QCPAxis::atTop) || ah.isAxisTypeSelected(customPlot, QCPAxis::atBottom) ) {
        ah.setAxesSelected(customPlot, QCPAxis::atTop);
        ah.setAxesSelected(customPlot, QCPAxis::atBottom);

        //set the zoom and drag ranges
        customPlot->axisRect()->setRangeZoom(customPlot->xAxis->orientation());
        customPlot->axisRect()->setRangeZoom(customPlot->xAxis->orientation());
    } else if ( ah.isAxisTypeSelected(customPlot, QCPAxis::atLeft) || ah.isAxisTypeSelected(customPlot, QCPAxis::atRight) ) {
        ah.setAxisSelected(customPlot->selectedAxes().first());

        //set the zoom and drag ranges
        customPlot->axisRect()->setRangeZoom(customPlot->yAxis->orientation());
        customPlot->axisRect()->setRangeZoom(customPlot->yAxis->orientation());

        customPlot->axisRect()->setRangeZoomAxes(customPlot->xAxis, customPlot->selectedAxes().first());
        customPlot->axisRect()->setRangeDragAxes(customPlot->xAxis, customPlot->selectedAxes().first());
    } else {
        customPlot->axisRect()->setRangeZoomAxes(customPlot->axisRect()->axes());
        customPlot->axisRect()->setRangeDragAxes(customPlot->axisRect()->axes());
    }
    QList<QCPAxis *> selectedAxes;
    selectedAxes.append(customPlot->xAxis);
    // synchronize selection of plottables with selection of corresponding legend items
    for (int i = 0; i < customPlot->plottableCount(); ++i) {
        QCPAbstractPlottable *plottable = customPlot->plottable(i);
        QCPPlottableLegendItem *item = customPlot->legend->itemWithPlottable(plottable);

        if (item->selected() || plottable->selected()) {
            item->setSelected(true);
            plottable->setSelection(QCPDataSelection(QCPDataRange(0, 1)));
        }

        if ( (!item->selected() && !plottable->selected()) &&
                (!customPlot->selectedPlottables().isEmpty() || !customPlot->selectedLegends().isEmpty()) ) {
            plottable->setPen(setPenAlpha(plottable->pen(), 64));
        } else {
            plottable->setPen(setPenAlpha(plottable->pen(), 192));
        }

        if (plottable->selected()) {
            selectedAxes.append(plottable->valueAxis());
        }
    }

    //If any plot is selected makes sure drag and zoom uses that plots axes
    if (!customPlot->selectedPlottables().empty()) {
        customPlot->axisRect()->setRangeZoomAxes(selectedAxes);
        customPlot->axisRect()->setRangeDragAxes(selectedAxes);
    }
}


void plot_interface::addToContextMenu(QMenu *menu, QCustomPlot *plot)
{
    QVariantMap metaDataMap;
    metaDataMap["Active Plot"] = qVariantFromValue( static_cast <void *>(plot));

    //If a plot is selected show the modify plot menu
    if ( ( !plot->selectedPlottables().empty() ) || (!plot->selectedItems().empty() ) || ah.isAxisSelected(plot, QCPAxis::atLeft) || ah.isAxisSelected(plot, QCPAxis::atRight) ) {
        menu->addAction( QIcon(":/graphics/pickNewLineColor.png"), tr("Pick Line Color"),
                         this, SLOT(selected_pickNewLineColor()) )->setData(metaDataMap);

        menu->addAction( QIcon(":/graphics/remove.png"),       tr("Remove Selected"),
                         this, SLOT(selected_remove()) )->setData(metaDataMap);
    }

    //If the xAxis is selected show the modify plot menu
    if ( ((plot->graphCount() > 0) && (!plot->selectedItems().empty() )) || ah.isAxisSelected(plot, QCPAxis::atBottom, QCPAxis::spTickLabels)) {
        menu->addAction( QIcon(":/graphics/axes.png"),       tr("Toggle Axis Format"),
                         this, SLOT(toggleAxisType()) )->setData(metaDataMap);
    }

    if ( !plot->selectedPlottables().empty() ) {
        QMenu *axesUnitsMenu = menu->addMenu (QIcon(":/graphics/axes.png"), tr("Axes") );

        int axisIndex = 0;
        metaDataMap["Axis Type"] = "Left";
        while (axisIndex < plot->axisRect()->axisCount(QCPAxis::atLeft)) {
            metaDataMap["Axis Index"] = axisIndex;

            QString menuText(tr("Use Left Y Axis"));
            menuText.append(QString(" %1").arg(axisIndex + 1));
            axesUnitsMenu->addAction( QIcon(":/graphics/leftAxis.png"),         menuText,
                                      this, SLOT(selected_changeAxis()) )->setData(metaDataMap);
            axisIndex++;
        }
        metaDataMap["Axis Index"] = axisIndex;
        axesUnitsMenu->addAction( QIcon(":/graphics/newLeftAxis.png"),         tr("New Left Y Axis"),
                                  this, SLOT(selected_changeAxis()) )->setData(metaDataMap);

        axisIndex = 0;
        metaDataMap["Axis Type"] = "Right";
        while (axisIndex < plot->axisRect()->axisCount(QCPAxis::atRight)) {
            metaDataMap["Axis Index"] = axisIndex;

            QString menuText(tr("Use Right Y Axis"));
            menuText.append(QString(" %1").arg(axisIndex + 1));
            axesUnitsMenu->addAction( QIcon(":/graphics/rightAxis.png"),         menuText,
                                      this, SLOT(selected_changeAxis()) )->setData(metaDataMap);
            axisIndex++;
        }
        metaDataMap["Axis Index"] = axisIndex;
        axesUnitsMenu->addAction( QIcon(":/graphics/newRightAxis.png"),        tr("New Right Y Axis"),
                                  this, SLOT(selected_changeAxis()) )->setData(metaDataMap);

        menu->addAction( QIcon(":/graphics/math.png"),             tr("Modify Data"),
                         this, SLOT(selected_modifyData()) )->setData(metaDataMap);

        //Add submenu for units conversion
        QMenu *convertUnitsMenu = menu->addMenu (QIcon(":/graphics/convert.png"), tr("Convert Units") );

        metaDataMap["Source Units"] = "degC";
        metaDataMap["Target Units"] = "degF";
        convertUnitsMenu->addAction(QIcon(":/graphics/temperature.png"),
                                    tr("Deg C -> Deg F"), this, SLOT(selectedPlot_convertUnits()))->setData(metaDataMap);

        metaDataMap["Source Units"] = "degF";
        metaDataMap["Target Units"] = "degC";
        convertUnitsMenu->addAction(QIcon(":/graphics/temperature.png"),
                                    tr("Deg C <- Deg F"), this, SLOT(selectedPlot_convertUnits()))->setData(metaDataMap);

        metaDataMap["Source Units"] = "grams";
        metaDataMap["Target Units"] = "lbs";
        convertUnitsMenu->addAction(QIcon(":/graphics/weight.png"),
                                    tr("Grams -> Lbs"), this, SLOT(selectedPlot_convertUnits()))->setData(metaDataMap);

        metaDataMap["Source Units"] = "lbs";
        metaDataMap["Target Units"] = "grams";
        convertUnitsMenu->addAction(QIcon(":/graphics/weight.png"),
                                    tr("Grams <- Lbs"), this, SLOT(selectedPlot_convertUnits()))->setData(metaDataMap);

        metaDataMap["Source Units"] = "bar";
        metaDataMap["Target Units"] = "psi";
        convertUnitsMenu->addAction(QIcon(":/graphics/pressure.png"),
                                    tr("Bar -> PSI"), this, SLOT(selectedPlot_convertUnits()))->setData(metaDataMap);

        metaDataMap["Source Units"] = "psi";
        metaDataMap["Target Units"] = "bar";
        convertUnitsMenu->addAction(QIcon(":/graphics/pressure.png"),
                                    tr("Bar <- PSI"), this, SLOT(selectedPlot_convertUnits()))->setData(metaDataMap);

        if (plot->selectedGraphs().empty()) {
            menu->addAction(QIcon(":/graphics/pickNewFillColor.png"), tr("Pick Fill Color"), this, SLOT(selectedPlot_pickNewFillColor()));
        } else if (plot->selectedGraphs().size() == 1) {
            menu->addAction(QIcon(":/graphics/rescalePlot.png"), tr("Rescale for selected graph"), this, SLOT(selected_rescaleGraph()))->setData(metaDataMap);
            menu->addAction(QIcon(":/graphics/metrics.png"), tr("Generate Stats"), this, SLOT(selectedPlot_stats()))->setData(metaDataMap);

            //Add submenu for convert plot
            QMenu *convertPlotMenu = menu->addMenu (QIcon(":/graphics/periodic.png"), tr("Convert to periodic") );
            //selectionData.clear();

            if (!plot->xAxis->ticker().dynamicCast<QCPAxisTickerDateTime>().isNull()) {
                metaDataMap["Interval Value"] = 1;

                metaDataMap["Interval Type"] = "Month";
                convertPlotMenu->addAction(QIcon(":/graphics/month.png"), tr("Month"),
                                           this, SLOT(selectedPlot_convert()))->setData(metaDataMap);

                metaDataMap["Interval Type"] = "Week";
                convertPlotMenu->addAction(QIcon(":/graphics/week.png"), tr("Week"),
                                           this, SLOT(selectedPlot_convert()))->setData(metaDataMap);

                metaDataMap["Interval Type"] = "Day";
                convertPlotMenu->addAction(QIcon(":/graphics/day.png"), tr("Day"),
                                           this, SLOT(selectedPlot_convert()))->setData(metaDataMap);

                metaDataMap["Interval Type"] = "Hour";
                convertPlotMenu->addAction(QIcon(":/graphics/hour.png"), tr("Hour"),
                                           this, SLOT(selectedPlot_convert()))->setData(metaDataMap);

                metaDataMap["Interval Type"] = "Minute";
                convertPlotMenu->addAction(QIcon(":/graphics/minute.png"), tr("Minute"),
                                           this, SLOT(selectedPlot_convert()))->setData(metaDataMap);

                metaDataMap["Interval Type"] = "Second";
                convertPlotMenu->addAction(QIcon(":/graphics/minute.png"), tr("Second"),
                                           this, SLOT(selectedPlot_convert()))->setData(metaDataMap);
            } else {
                metaDataMap["Interval Type"] = "Count";

                metaDataMap["Interval Value"] = 1000;
                convertPlotMenu->addAction("1,000", this, SLOT(selectedPlot_convert()))->setData(metaDataMap);

                metaDataMap["Interval Value"] = 100;
                convertPlotMenu->addAction("100", this, SLOT(selectedPlot_convert()))->setData(metaDataMap);

                metaDataMap["Interval Value"] = 10;
                convertPlotMenu->addAction("10", this, SLOT(selectedPlot_convert()))->setData(metaDataMap);

                metaDataMap["Interval Value"] = 1;
                convertPlotMenu->addAction("1", this, SLOT(selectedPlot_convert()))->setData(metaDataMap);
            }
        }
    }
    if ( plot->plottableCount() > 0 )
        menu->addAction(tr("Remove all graphs"), this, SLOT(removeAll()))->setData(metaDataMap);
}

void plot_interface::selected_remove()
{
    // make sure this slot is really called by a context menu action, so it carries the data we need
    if (auto *contextAction = qobject_cast<QAction *>(sender())) {
        QVariantMap metaDataMap = contextAction->data().toMap();

        QCustomPlot *activePlot = static_cast <QCustomPlot *>(metaDataMap["Active Plot"].value<void *>());

        //Delete all selected and unused axis
        for (int axis = 0 ; axis < activePlot->selectedAxes().size() ; axis++) {
            //Don't allow deletion of the primary y axis
            if (activePlot->yAxis == activePlot->selectedAxes().value(axis))
                continue;

            if ( (activePlot->selectedAxes().value(axis)->axisType() == QCPAxis::atLeft) ||
                    (activePlot->selectedAxes().value(axis)->axisType() == QCPAxis::atRight) ) {
                bool axisInUse = false;
                for (int plottable = 0 ; plottable < activePlot->plottableCount() ; plottable++) {
                    //QCustomplot dies if we delete the axis being used by a plot
                    if (activePlot->plottable(plottable)->valueAxis() == activePlot->selectedAxes().value(axis)) {
                        axisInUse = true;
                        break;
                    }
                }

                if (!axisInUse) {
                    activePlot->axisRect()->removeAxis(activePlot->selectedAxes().value(axis));
                    axis--; //Adjust the index for the removed axis
                }
            }
        }

        while (!activePlot->selectedPlottables().isEmpty()) {
            activePlot->removePlottable(activePlot->selectedPlottables().first());
        }

        while (!activePlot->selectedItems().isEmpty()) {
            activePlot->removeItem(activePlot->selectedItems().first());
        }

        selectionChanged(activePlot);
        activePlot->replot();
    }
}

void plot_interface::selected_rescaleGraph()
{
    // make sure this slot is really called by a context menu action, so it carries the data we need
    if (auto *contextAction = qobject_cast<QAction *>(sender())) {
        QVariantMap metaDataMap = contextAction->data().toMap();

        QCustomPlot *activePlot = static_cast <QCustomPlot *>(metaDataMap["Active Plot"].value<void *>());

        if (!activePlot->selectedPlottables().empty()) {
            activePlot->selectedPlottables().first()->rescaleValueAxis();

            //Only rescale key axis if no other plot exisits
            if (activePlot->plottableCount() == 1) {
                activePlot->selectedPlottables().first()->rescaleKeyAxis();
            }
        }
        activePlot->replot();
    }
}

void plot_interface::selected_pickNewLineColor()
{
    // make sure this slot is really called by a context menu action, so it carries the data we need
    if (auto *contextAction = qobject_cast<QAction *>(sender())) {
        QVariantMap metaDataMap = contextAction->data().toMap();

        QCustomPlot *activePlot = static_cast <QCustomPlot *>(metaDataMap["Active Plot"].value<void *>());

        QPen graphPen;
        graphPen.setColor(QColorDialog::getColor(Qt::green, nullptr));
        graphPen.setStyle(Qt::SolidLine);
        graphPen.setWidthF(1);

        for (int plottable = 0 ; plottable < activePlot->selectedPlottables().size() ; plottable++) {
            activePlot->selectedPlottables().value(plottable)->setPen(graphPen);
            //activePlot->selectedPlottables().value(plottable)->setSelectedPen(graphPen);
        }

        for (int axis = 0 ; axis < activePlot->selectedAxes().size() ; axis++) {
            activePlot->selectedAxes().value(axis)->setBasePen(graphPen);
            activePlot->selectedAxes().value(axis)->setTickPen(graphPen);
            activePlot->selectedAxes().value(axis)->setSubTickPen(graphPen);
        }

        activePlot->replot();
    }
}

void plot_interface::selected_pickNewFillColor()
{
    // make sure this slot is really called by a context menu action, so it carries the data we need
    if (auto *contextAction = qobject_cast<QAction *>(sender())) {
        QVariantMap metaDataMap = contextAction->data().toMap();

        QCustomPlot *activePlot = static_cast <QCustomPlot *>(metaDataMap["Active Plot"].value<void *>());

        for (int plottable = 0 ; plottable < activePlot->selectedPlottables().size() ; plottable++) {
            activePlot->selectedPlottables().value(plottable)->setBrush(QBrush(QColorDialog::getColor(Qt::green, nullptr, QString(""), QColorDialog::ShowAlphaChannel)));
        }
        activePlot->replot();
    }
}

void plot_interface::selected_changeAxis()
{
    // make sure this slot is really called by a context menu action, so it carries the data we need
    if (auto *contextAction = qobject_cast<QAction *>(sender())) {
        QVariantMap metaDataMap = contextAction->data().toMap();

        QCustomPlot *activePlot = static_cast <QCustomPlot *>(metaDataMap["Active Plot"].value<void *>());

        for (int plottable = 0 ; plottable < activePlot->selectedPlottables().size() ; plottable++) {
            if (metaDataMap.value("Axis Type") == "Right") {
                //We are moving to an existing axis
                if (metaDataMap.value("Axis Index").toInt() < activePlot->axisRect()->axisCount(QCPAxis::atRight) ) {

                    QCPAxis *targetAxis = activePlot->axisRect()->axis(QCPAxis::atRight, metaDataMap.value("Axis Index").toInt());
                    targetAxis->setTickLabels(true);
                    activePlot->selectedPlottables().value(plottable)->setValueAxis(targetAxis);
                }
                //The target axis does not exist, lets make that baby
                else {
                    QCPAxis *newAxis = activePlot->axisRect()->addAxis(QCPAxis::atRight);
                    newAxis->setTickLabels(true);
                    activePlot->selectedPlottables().value(plottable)->setValueAxis(newAxis);
                }
            } else {
                //We are moving to an existing axis
                if (metaDataMap.value("Axis Index").toInt() < activePlot->axisRect()->axisCount(QCPAxis::atLeft) ) {

                    QCPAxis *targetAxis = activePlot->axisRect()->axis(QCPAxis::atLeft, metaDataMap.value("Axis Index").toInt());
                    targetAxis->setTickLabels(true);
                    activePlot->selectedPlottables().value(plottable)->setValueAxis(targetAxis);
                }
                //The target axis does not exist, lets make that baby
                else {
                    QCPAxis *newAxis = activePlot->axisRect()->addAxis(QCPAxis::atLeft);
                    newAxis->setTickLabels(true);
                    activePlot->selectedPlottables().value(plottable)->setValueAxis(newAxis);
                }
            }
        }
        //rescale takes care of replot
        selected_rescaleGraph();
    }
}

void plot_interface::selectedPlot_stats()
{
    // make sure this slot is really called by a context menu action, so it carries the data we need
    if (auto *contextAction = qobject_cast<QAction *>(sender())) {
        QVariantMap metaDataMap = contextAction->data().toMap();

        QCustomPlot *activePlot = static_cast <QCustomPlot *>(metaDataMap["Active Plot"].value<void *>());

        if (!activePlot->selectedGraphs().empty()) {
            plot_analytics analytics;
            plotStats stats;
            //activePlot->selectedGraphs().first()
            analytics.plotAnalyze(activePlot->selectedGraphs().first(), &stats);
            //HTML not supported in mobile OSes
            //How to handle this long term?
            QMessageBox::about(nullptr, tr("Plot Stats"),
                               QString(  "Total Time:               %L1\n"
                                         "Sum of Changes:           %L2\n"
                                         "Max Value:                %L3\n"
                                         "Min Value:                %L4\n"
                                         "Max P2P(Point to Point)Δ: %L5\n"
                                         "Min P2PΔ:                 %L6\n"
                                         "Avg P2PΔ/S:               %L7\n"
                                         "Max Time Of No P2PΔ:      %L8\n"
                                         "Max Time Of P2PΔ:         %L9\n"
                                         "Total Time Of No P2PΔ:    %L10\n"
                                         "Total Time Of P2PΔ:       %L11").
                               arg(seconds_to_DHMS(stats.totalData_seconds)).
                               arg(stats.totalValue_DeltaP2P).
                               arg(stats.maxValue).
                               arg(stats.minValue).
                               arg(stats.maxPosValue_DeltaP2P).
                               arg(stats.minValue_DeltaP2P).
                               arg(stats.totalValue_DeltaP2P / stats.totalData_seconds).
                               arg(seconds_to_DHMS(stats.longestTimeNoDelta_seconds)).
                               arg(seconds_to_DHMS(stats.longestTimeDelta_seconds)).
                               arg(seconds_to_DHMS(stats.totalTimeNoDelta_seconds)).
                               arg(seconds_to_DHMS(stats.totalTimeDelta_seconds)) );
        }
    }
}

void plot_interface::selectedPlot_convertUnits()
{
    // make sure this slot is really called by a context menu action, so it carries the data we need
    if (auto *contextAction = qobject_cast<QAction *>(sender())) {
        QVariantMap metaDataMap = contextAction->data().toMap();

        QCustomPlot *activePlot = static_cast <QCustomPlot *>(metaDataMap["Active Plot"].value<void *>());

        if (!activePlot->selectedGraphs().empty()) {
            QString conversionFunciton;

            if ( (metaDataMap.value("Source Units").toString() == "degC") && (metaDataMap.value("Target Units").toString() == "degF") )
                conversionFunciton.append("(x * 9/5) + 32");
            else if ( (metaDataMap.value("Source Units").toString() == "degF") && (metaDataMap.value("Target Units").toString() == "degC") )
                conversionFunciton.append("(x - 32) * 5/9");
            else if ( (metaDataMap.value("Source Units").toString() == "grams") && (metaDataMap.value("Target Units").toString() == "lbs") )
                conversionFunciton.append("x * 0.00220462");
            else if ( (metaDataMap.value("Source Units").toString() == "lbs") && (metaDataMap.value("Target Units").toString() == "grams") )
                conversionFunciton.append("x * 453.592");
            else if ( (metaDataMap.value("Source Units").toString() == "bar") && (metaDataMap.value("Target Units").toString() == "psi") )
                conversionFunciton.append("x * 14.5037738");
            else if ( (metaDataMap.value("Source Units").toString() == "psi") && (metaDataMap.value("Target Units").toString() == "bar") )
                conversionFunciton.append("x * 0.0689475729");
            else
                return;

            for (int graph = 0 ; graph < activePlot->selectedGraphs().size() ; graph++) {
                ph.plotConvert( activePlot->selectedGraphs().value(graph), conversionFunciton);
            }

            ah.updateGraphAxes(activePlot);
        }
    }
}

void plot_interface::removeAll()
{
    // make sure this slot is really called by a context menu action, so it carries the data we need
    if (auto *contextAction = qobject_cast<QAction *>(sender())) {
        QVariantMap metaDataMap = contextAction->data().toMap();

        QCustomPlot *activePlot = static_cast <QCustomPlot *>(metaDataMap["Active Plot"].value<void *>());

        activePlot->clearGraphs();
        activePlot->clearPlottables();
        activePlot->clearItems();
        activePlot->replot();
    }
}

void plot_interface::toggleAxisType()
{
    // make sure this slot is really called by a context menu action, so it carries the data we need
    if (auto *contextAction = qobject_cast<QAction *>(sender())) {
        QVariantMap metaDataMap = contextAction->data().toMap();

        QCustomPlot *activePlot = static_cast <QCustomPlot *>(metaDataMap["Active Plot"].value<void *>());

        ah.toggleAxisType(activePlot->xAxis);
        //ah.updateAxisTickCount(activePlot, this->parent());
        ah.updateGraphAxes(activePlot);
    }
}

void plot_interface::selectedPlot_convert()
{
    if (auto *contextAction = qobject_cast<QAction *>(sender())) {
        QVariantMap metaDataMap = contextAction->data().toMap();

        QCustomPlot *activePlot = static_cast <QCustomPlot *>(metaDataMap["Active Plot"].value<void *>());

        if (!activePlot->selectedGraphs().empty()) {
            ph.plotAddPeriodicReport( activePlot->selectedGraphs().first(),  metaDataMap );
            selected_remove();
            ah.updateGraphAxes(activePlot);
        }
    }
}

//Turns out the first user selected plot is the last in the selected list....
//TODO: This currently uses the first selected plot to drive point intervals, should probably pick the one with the most points..
void plot_interface::selected_modifyData()
{
    if (auto *contextAction = qobject_cast<QAction *>(sender())) {
        QVariantMap metaDataMap = contextAction->data().toMap();

        QCustomPlot *activePlot = static_cast <QCustomPlot *>(metaDataMap["Active Plot"].value<void *>());

        QVector<double> x, y;
        QString functionString("(function(");
        bool ok;

        if (!activePlot->selectedPlottables().empty()) {
            QString messageBoxText(tr("Enter Equation:"));
            int symbolChar = 122; //z

            //Create a vector containing iterators for all the data sources
            QVector<QCPGraphDataContainer::const_iterator> graphData;
            QVector<QCPBarsDataContainer::iterator> barData;
            QCPBars *firstBar;
            QCPGraph *firstGraph;

            for (int plot = 0 ; plot < activePlot->selectedPlottables().size() ; plot++) {
                auto *currentBar =  qobject_cast<QCPBars *>(activePlot->selectedPlottables().value(plot));
                auto *currentGraph =  qobject_cast<QCPGraph *>(activePlot->selectedPlottables().value(plot));

                if ((currentBar != nullptr) && (graphData.empty())) {
                    if (barData.empty())
                        firstBar = currentBar;

                    //Only do math on matching data sets, we will assume if the starting key values are the same and the number of key values are the
                    //same, they are part of the same set
                    if ( firstBar->data()->size() == currentBar->data()->size() )
                        barData.append(currentBar->data()->begin());
                    else
                        continue;
                } else if ((currentGraph != nullptr) && (barData.empty())) {
                    if (graphData.empty())
                        firstGraph = currentGraph;

                    graphData.append(currentGraph->data()->constBegin());
                } else
                    continue;

                //Let the user know which variable will represent data from this plot
                messageBoxText.append( QString("\n%1=%2").arg(QChar(symbolChar), activePlot->selectedPlottables().value(plot)->name()) );
                //Add the varible to our function
                functionString.append(QString("%1, ").arg(QChar(symbolChar)));
                //Get a new symbol char
                if (symbolChar > 0)
                    symbolChar--;
            }

            //Strip off last ', '
            functionString = functionString.left(functionString.size() - 2);

            //Complete the function string with user input
            functionString.append(") { return ");
            QString userString = QInputDialog::getText(nullptr, tr("SmartPlot"), messageBoxText, QLineEdit::Normal, "", &ok, (Qt::WindowCloseButtonHint | Qt::WindowStaysOnTopHint) );
            functionString.append(userString);
            functionString.append("; })");

            if (!ok && userString.isEmpty())
                return;

            //qDebug() << "Function String: " << functionString;
            QJSEngine plotEngine;
            QJSValue plotFunction = plotEngine.evaluate(functionString);
            QJSValueList plotValue;

            if (!graphData.empty()) {
                while (graphData.first() != firstGraph->data()->constEnd()) {
                    plotValue.clear();
                    x.append(graphData.first()->key);
                    //populate QScriptValueList with data from each graph for the plotFunction call
                    for (int graph = 0 ; graph < graphData.size() ; graph++) {
                        auto *currentGraph =  qobject_cast<QCPGraph *>(activePlot->selectedPlottables().value(graph));
                        QCPGraphDataContainer::const_iterator refIterator = currentGraph->data().data()->findBegin(x.last(), false);
                        if (refIterator != currentGraph->data().data()->constEnd()) {
                            plotValue << refIterator->value;
                        } else {
                            plotValue << 0;
                        }

                        graphData[graph]++;
                    }
                    //Make sure the result is not infinite or non-existant
                    if (isInvalidData(plotFunction.call(plotValue).toNumber()))
                        y.append(std::numeric_limits<double>::quiet_NaN());
                    else
                        y.append(plotFunction.call(plotValue).toNumber());
                }
                //Replace target plot with new data
                firstGraph->setData(x, y);
                firstGraph->setName(firstGraph->name() + "*");
            } else if (!barData.empty()) {
                while (barData.first() != firstBar->data()->constEnd()) {
                    plotValue.clear();
                    for (int bar = 0 ; bar < barData.size() ; bar++) {
                        plotValue << barData.value(bar)->value;
                    }

                    barData.first()->value = plotFunction.call(plotValue).toNumber();
                    //Make sure the result is not infinite or non-existant
                    if (isInvalidData(barData.first()->value))
                        barData.first()->value = 0;//std::numeric_limits<double>::quiet_NaN();

                    for (auto &bar : barData) {
                        bar++;
                    }
                }
            }
            activePlot->replot();
        }
    }
}
