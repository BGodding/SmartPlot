#include "smart_plot.h"

#include "tabs/plot/plot_analytics.h"
#include "utility.h"

#include <QTranslator>

smart_plot::smart_plot(QWidget *parent) :
    QMainWindow(parent)
{
    myUrlHandler *myHandler = new myUrlHandler();
    QDesktopServices::setUrlHandler("file", myHandler, "files");
    connect(myHandler, SIGNAL(openFile(const QString&)), this, SLOT( iosOpen(const QString &)));

    setWindowTitle(tr("Smartplot Ver 2.01.005"));

    QCoreApplication::setOrganizationName("bgodding");
    QCoreApplication::setApplicationName("smartplot");

    this->setMinimumSize(320,240);
    this->resize(1024, 600);
    this->installEventFilter(this);

    //Setup the main menu
    QMenu *fileMenu = menuBar()->addMenu(tr("&File"));

    //Data Handlers: Add menu calls here
    csvHandler.addToSystemMenu(fileMenu);
    clocHandler.addToSystemMenu(fileMenu);

    fileMenu->addSeparator();

    fileMenu->addSeparator();
    fileMenu->addAction( QIcon(":/graphics/savePlot.png"), tr("&SaveImage"), this, SLOT(savePlotAsImage()) );

    plots.append(new QCustomPlot(this));

    setCentralWidget(activePlot());

    initGraph(activePlot());
    initLegend(activePlot());

#ifdef MOBILE
    setAttribute( Qt::WA_AcceptTouchEvents );
    grabGesture( Qt::TapAndHoldGesture );

//    //TODO: Only use these on mobile in lou of pinch to zoom on crappy hardware?
    QPushButton *zoomIn = new QPushButton("+", this);
    zoomIn->setObjectName("+");
    connect(zoomIn, SIGNAL(released()), this, SLOT(zoomInButtonPressed()));

    QPushButton *zoomOut = new QPushButton("-", this);
    zoomOut->setObjectName("-");
    connect(zoomOut, SIGNAL(released()), this, SLOT(zoomOutButtonPressed()));
#endif

    contextMenu = new QMenu(this);

    this->installEventFilter(this);
}

void smart_plot::zoomInButtonPressed()
{
    QList<QCPAxis*> axes = activePlot()->axisRect()->axes();

    int axisIndex = 0;
    while(axisIndex < axes.size())
    {
        if ( (activePlot()->selectedAxes().size() == 0) || axisHandler.isAxisSelected( axes.value(axisIndex) ) )
        {
            axes.value(axisIndex)->scaleRange(.5,axes.value(axisIndex)->range().center());
        }
        axisIndex++;
    }
    activePlot()->replot();
}

void smart_plot::zoomOutButtonPressed()
{
    QList<QCPAxis*> axes = activePlot()->axisRect()->axes();

    int axisIndex = 0;
    while(axisIndex < axes.size())
    {
        if ( (activePlot()->selectedAxes().size() == 0) || axisHandler.isAxisSelected( axes.value(axisIndex) ) )
        {
            axes.value(axisIndex)->scaleRange(1.5,axes.value(axisIndex)->range().center());
        }
        axisIndex++;
    }

    activePlot()->replot();
}

void smart_plot::savePlotAsImage()
{
    QSettings settings;
    QString fileName = QFileDialog::getSaveFileName (this, tr("Save Graph"),
                                                     settings.value("Plot Image Export Directory").toString(), "PNG (*.png);;PDF (*.PDF)");

    if(!fileName.isEmpty())
    {
        //Remember directory
        settings.setValue("Plot Image Export Directory", QFileInfo(fileName).absolutePath());

        //Determine how to save the file
        if(fileName.right(3)=="PDF")
            activePlot()->savePdf(fileName);
        else
            activePlot()->savePng(fileName);
    }
}

void smart_plot::initLegend(QCustomPlot *customPlot)
{
    customPlot->legend->setVisible(true);
    QFont legendFont("Monospace");
    legendFont.setPointSize(12);
    customPlot->legend->setFont(legendFont);
    customPlot->legend->setSelectedFont(legendFont);
    // legend box shall not be selectable, only legend items
    customPlot->legend->setSelectableParts(QCPLegend::spItems);
}

void smart_plot::initGraph(QCustomPlot *customPlot)
{
    //Adds axes on the top and right
    customPlot->axisRect()->setupFullAxesBox();

    //Add title
    customPlot->plotLayout()->insertRow(0);
    QCPTextElement *title = new QCPTextElement(customPlot, tr("Open Files to Plot Data"), QFont("sans", 17, QFont::Bold));
    customPlot->plotLayout()->addElement(0, 0, title);

    //Add axes labels
    customPlot->xAxis->setLabel(tr("X Axis"));
    customPlot->yAxis->setLabel(tr("Y Axis"));

    customPlot->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom | QCP::iSelectAxes |
                                    QCP::iSelectLegend | QCP::iSelectPlottables | QCP::iSelectItems | QCP::iMultiSelect);

    connect(customPlot, SIGNAL(selectionChangedByUser()),
            this, SLOT(selectionChanged()));

    connect(customPlot->xAxis,SIGNAL(rangeChanged(QCPRange)),
            this, SLOT(rangeChanged(QCPRange)));

    connect(customPlot, SIGNAL(mousePress(QMouseEvent*)),
            this, SLOT(mousePress(QMouseEvent*)));

    connect(title, SIGNAL(doubleClicked(QMouseEvent*)), this, SLOT(titleDoubleClick(QMouseEvent*)));

    connect(customPlot, SIGNAL(axisDoubleClick(QCPAxis*,QCPAxis::SelectablePart,QMouseEvent*)),
            this, SLOT(axisDoubleClick(QCPAxis*,QCPAxis::SelectablePart)));

    connect(customPlot, SIGNAL(legendDoubleClick(QCPLegend*,QCPAbstractLegendItem*,QMouseEvent*)),
            this, SLOT(legendDoubleClick(QCPLegend*,QCPAbstractLegendItem*)));

    // setup policy and connect slot for context menu popup:
    customPlot->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(customPlot, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(contextMenuRequest(QPoint)));

    // make bottom x axis transfer its ranges to top x axis which is used for showing events
    connect(customPlot->xAxis, SIGNAL(rangeChanged(QCPRange)), customPlot->xAxis2, SLOT(setRange(QCPRange)));

    //Short Cuts!
    //Rescale to first selected plot
    //new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_R), this, SLOT(close()));
    //Rescale to first selected plot
    //new QShortcut(Qt::Key_Delete, this, SLOT(selected_remove()));
}

void smart_plot::rangeChanged(const QCPRange &newRange)
{
    //With 10K+ events on multiple axes this will get pretty slow.
    //Maybe only refresh if zoom changes by +/- 10%?
    //Data Handlers: Add function calls here to update tick axes if desired
    csvHandler.updateAxis(activePlot());
}

void smart_plot::mousePress(QMouseEvent *event)
{
    static double prevKey, prevValue;

    if(event->button() == Qt::MiddleButton)
    {
        QCPAbstractPlottable *plottable = activePlot()->plottableAt(event->localPos());

        if(plottable)
        {
            QCPGraph *graph = qobject_cast<QCPGraph*>(plottable);
            plot_analytics analytics;
            plotStats stats;

            if(graph)
            {
                //Do diff by % range vs
                double mouseKey = graph->keyAxis()->pixelToCoord(event->localPos().x());
                double mouseValue = graph->valueAxis()->pixelToCoord(event->localPos().y());
                double keyRange = graph->keyAxis()->range().size();
                double keyDistance_no_abs = 0;
                double value = 0;
                double key = 0;

                bool ok = false;
                double m = std::numeric_limits<double>::max();
                //QCPGraphDataContainer
                analytics.plotAnalyze( graph, &stats, graph->keyAxis()->range());

                //Iterate through on screen data and see which point is closest
                QCPGraphDataContainer::const_iterator QCPGraphDataBegin = graph->data().data()->findBegin(graph->keyAxis()->range().lower,true);
                QCPGraphDataContainer::const_iterator QCPGraphDataEnd = graph->data().data()->findEnd(graph->keyAxis()->range().upper,true);
                for (QCPGraphDataContainer::const_iterator QCPGraphDataIt=QCPGraphDataBegin; QCPGraphDataIt!=QCPGraphDataEnd; ++QCPGraphDataIt)
                {
                    double valueRange = graph->valueAxis()->range().size();
                    double keyDistance = qAbs(mouseKey - QCPGraphDataIt->key)/keyRange;
                    double valueDistance = qAbs(mouseValue - QCPGraphDataIt->value)/valueRange;

                    if( (valueDistance + keyDistance) < m )
                    {
                        value = QCPGraphDataIt->value;
                        key = QCPGraphDataIt->key;
                        keyDistance_no_abs = mouseKey - QCPGraphDataIt->key;
                        ok = true;
                        m = (valueDistance + keyDistance);
                    }
                }
//                qDebug () << QDateTime::fromTime_t((int)mouseKey) << value;

                if(ok)
                {
                    QToolTip::hideText();

                    if(!qSharedPointerDynamicCast<QCPAxisTickerDateTime>(graph->keyAxis()->ticker()).isNull())
                    {
                        if(QApplication::keyboardModifiers().testFlag(Qt::ControlModifier))
                        {
                            quint32 timeDelta = qAbs(mouseKey-prevKey);

                            // add the bracket at the top:
                            QCPItemBracket *bracket = new QCPItemBracket(activePlot());
                            bracket->left->setAxes(graph->keyAxis(), graph->valueAxis());
                            bracket->left->setCoords(prevKey, value);
                            bracket->right->setAxes(graph->keyAxis(), graph->valueAxis());
                            bracket->right->setCoords(mouseKey, value);

                            // add the text label at the top:
                            QCPItemText *wavePacketText = new QCPItemText(activePlot());
                            wavePacketText->position->setParentAnchor(bracket->center);
                            wavePacketText->position->setCoords(0, -10); // move 10 pixels to the top from bracket center anchor
                            wavePacketText->setPositionAlignment(Qt::AlignBottom|Qt::AlignHCenter);
                            wavePacketText->setText(
                                QString("%L1: ΔX->%L2 ΔY->%L3").
                                   arg(graph->name().isEmpty() ? "..." : graph->name()).
                                   arg(seconds_to_DHMS(timeDelta)).
                                   arg(value-prevValue));
                            wavePacketText->setFont(QFont(font().family(), 12));
                            activePlot()->replot();
                        }
                        else if(QApplication::keyboardModifiers().testFlag(Qt::AltModifier))
                        {
                            if(QApplication::keyboardModifiers().testFlag(Qt::ShiftModifier))
                                graph->addData(graph->keyAxis()->pixelToCoord(event->localPos().x()),
                                               graph->valueAxis()->pixelToCoord(event->localPos().y()));
                            else if(keyDistance_no_abs < 0)
                                graph->addData(key - 1, std::numeric_limits<double>::quiet_NaN());
                            else
                                graph->addData(key + 1, std::numeric_limits<double>::quiet_NaN());

                            activePlot()->replot();
                        }
                        else if(QApplication::keyboardModifiers().testFlag(Qt::ShiftModifier))
                        {
                            //Delete point
                            graph->data().data()->remove(key);
                            activePlot()->replot();
                        }
                        //Hold Alt to insert NAN(Break link?)
                        else
                        {
                            QDateTime dateTime;
                            dateTime.setTime_t(mouseKey);

                            //activePlot()->xAxis->tick
                            QToolTip::showText(event->globalPos(),
                            QString("<table>"
                                    "<tr>" "<th colspan=\"2\">%L1</th>"  "</tr>"
                                    "<tr>" "<td>X:</td>"   "<td>%L2</td>" "</tr>"
                                    "<tr>" "<td>Y:</td>"   "<td>%L3</td>" "</tr>"
                                    "<tr>" "<td>AVG:</td>" "<td>%L4</td>" "</tr>"
                                   "</table>").
                               arg(graph->name().isEmpty() ? "..." : graph->name()).
                               arg(dateTime.toString(qSharedPointerDynamicCast<QCPAxisTickerDateTime>(graph->keyAxis()->ticker())->dateTimeFormat())).
                               arg(value).
                               arg(stats.avgValue),
                               activePlot(), activePlot()->rect());
                        }
                    }
                    else
                    {
                        QToolTip::showText(event->globalPos(),
                        QString("<table>"
                                "<tr>" "<th colspan=\"2\">%L1</th>"  "</tr>"
                                "<tr>" "<td>X:</td>"   "<td>%L2</td>" "</tr>"
                                "<tr>" "<td>Y:</td>"   "<td>%L3</td>" "</tr>"
                                "<tr>" "<td>AVG:</td>" "<td>%L4</td>" "</tr>"
                               "</table>").
                           arg(graph->name().isEmpty() ? "..." : graph->name()).
                           arg(mouseKey).
                           arg(value).
                           arg(stats.avgValue),
                           activePlot(), activePlot()->rect());
                    }
                }

                prevKey = mouseKey;
                prevValue = value;
            }
        }
    }
}


void smart_plot::selectionChanged()
{
    plotInterface.selectionChanged(activePlot());
}

void smart_plot::contextMenuRequest(QPoint pos)
{
    contextMenu->clear();
    contextMenu->installEventFilter(this);

    plotInterface.addToContextMenu(contextMenu, activePlot());

    csvHandler.addToContextMenu(contextMenu, activePlot());
    clocHandler.addToContextMenu(contextMenu, activePlot());

    contextMenu->addSeparator();
    influxdbHandler.addToContextMenu(contextMenu, activePlot());

    if(!contextMenu->isEmpty())
        contextMenu->popup(activePlot()->mapToGlobal(pos));
}

void smart_plot::titleDoubleClick(QMouseEvent* event)
{
  Q_UNUSED(event)
  if (QCPTextElement *title = qobject_cast<QCPTextElement*>(sender()))
  {
    // Set the plot title by double clicking on it
    bool ok;
    QString newTitle = QInputDialog::getText(this, tr("SmartPlot"), tr("New plot title:"), QLineEdit::Normal, title->text(), &ok, (Qt::WindowCloseButtonHint | Qt::WindowStaysOnTopHint));
    if (ok)
    {
        title->setText(newTitle);

        #if FILE_DATABASE
            tabs->setTabText(tabs->currentIndex(), newTitle);
        #endif //FILE_DATABASE

        activePlot()->replot();
    }
  }
}

void smart_plot::axisDoubleClick(QCPAxis *axis, QCPAxis::SelectablePart part)
{
    // only react when the actual axis label is clicked, not tick label or axis backbone
    if (part == QCPAxis::spAxisLabel)
    {
        bool ok;
        QString newLabel = QInputDialog::getText(this, tr("SmartPlot"), tr("New axis label:"), QLineEdit::Normal, axis->label(), &ok, (Qt::WindowCloseButtonHint | Qt::WindowStaysOnTopHint));
        if (ok)
        {
            axis->setLabel(newLabel);
            activePlot()->replot();
        }
    }
    else if (part == QCPAxis::spTickLabels)
    {
        QSettings settings;
        plot_interface::tickerType currentLabelType = static_cast<plot_interface::tickerType>(settings.value("X Axis Tick Label Format", plot_interface::dateTime).toInt());
        plot_interface::tickerType newLabelType;

        if(currentLabelType == plot_interface::dateTime)
            newLabelType = plot_interface::fixed;
        else// if(currentAxisType == plot_interface::fixed)
            newLabelType = plot_interface::dateTime;

        settings.setValue("X Axis Tick Label Format", newLabelType);
        axisHandler.updateGraphAxes(activePlot());
    }
}

void smart_plot::legendDoubleClick(QCPLegend *legend, QCPAbstractLegendItem *item)
{
    Q_UNUSED(legend)
    // only react if item was clicked (user could have clicked on border padding of legend where there is no item, then item is 0)
    if (item)
    {
        bool ok;
        QCPPlottableLegendItem *plItem = qobject_cast<QCPPlottableLegendItem*>(item);
        QString newName = QInputDialog::getText(this, tr("SmartPlot"), tr("New graph name:"), QLineEdit::Normal, plItem->plottable()->name(), &ok, (Qt::WindowCloseButtonHint | Qt::WindowStaysOnTopHint));
        if (ok)
        {
            plItem->plottable()->setName(newName);
            activePlot()->replot();
        }
    }
}

void smart_plot::iosOpen(const QString & fileName)
{
    if ( (fileName.isEmpty() == false) )// && (str.contains(".CSV")) )
    {
        url = fileName;
    }
}

bool smart_plot::eventFilter(QObject* object,QEvent* event)
{
    //qDebug() << "EF" << event->type();
    if(!url.isEmpty())
    {
        qDebug() << "URL: " << url;
        QVariantMap modifier;
        modifier["File Name"] = url;
        url.clear();

        QMessageBox msgBox(this);
        msgBox.setWindowFlags(Qt::Dialog);

        msgBox.setText(tr("Plase select the file type"));
        msgBox.setInformativeText(modifier.value("File Name").toString());

        QPushButton *dataFileButton = csvHandler.addToMessageBox(msgBox);

        msgBox.exec();

        if(msgBox.clickedButton() == dataFileButton)
        {
            csvHandler.dataImport(modifier);
        }
    }

    if ( event->type() == QEvent::MouseButtonRelease )
    {
        QMenu *objectMenu = qobject_cast<QMenu*>(object);

        //Check if object is actually a menu
        if(objectMenu != nullptr)
        {
            QAction *menuAction = objectMenu->activeAction();
            //Check if the selected item has an action
            if(menuAction != nullptr)
            {
                objectMenu->activeAction()->trigger();

                objectMenu->update();
                //Returning true prevents the standard event processing and keeps the menu open
                return true;
            }
        }
    }
    else if ( event->type() == QEvent::KeyPress )
    {
        int key = static_cast<QKeyEvent*>(event)->key();
        if(key == Qt::Key_F11)
        {
            if(!this->isFullScreen())
                this->showFullScreen();
            else
                this->showNormal();
            return true;
        }
    }
    else if( event->type() == QEvent::Gesture)
    {
        QGestureEvent *gestureEve = static_cast<QGestureEvent*>(event);
        if( QGesture *tapAndHold = gestureEve->gesture(Qt::TapAndHoldGesture) )
        {
            if( (contextMenu != nullptr) && !contextMenu->isVisible())
            {
                contextMenuRequest(activePlot()->mapFromGlobal(tapAndHold->hotSpot().toPoint()));
            }
        }
    }
    return false;
}

void smart_plot::resizeEvent(QResizeEvent* event)
{
    Q_UNUSED(event);
    //qDebug() << "Pixel Ratio:" << devicePixelRatio() << this->width();
    //Account for higher DPI on mobile
    #ifdef MOBILE

    int buttonHeight = 60/devicePixelRatio();
    int buttonWidth = 120/devicePixelRatio();

    activePlot()->xAxis->setAutoTickCount(this->width()*devicePixelRatio()/250);

    QSize screenSize = QApplication::primaryScreen()->availableSize();

    QList<QPushButton *> allPButtons = this->findChildren<QPushButton *>("+");

    if(!allPButtons.isEmpty())
        allPButtons.first()->setGeometry( (screenSize.width()-buttonWidth), (screenSize.height()-buttonHeight), buttonWidth, buttonHeight);

    allPButtons = this->findChildren<QPushButton *>("-");

    if(!allPButtons.isEmpty())
        allPButtons.first()->setGeometry( 0, (screenSize.height()-buttonHeight), buttonWidth, buttonHeight );
    #else
    //activePlot()->xAxis->setAutoTickCount(this->width()/200);
    #endif
}

void smart_plot::closeEvent(QCloseEvent *)
{
    influxdbHandler.close();
}

QCustomPlot* smart_plot::activePlot()
{
    int currentTab = 0xFFFF;

    #if FILE_DATABASE
        currentTab = tabs->currentIndex();
    #endif //FILE_DATABASE

    if( (currentTab >=0) && (currentTab < plots.size()))
        return plots.value(currentTab);
    else if (!plots.isEmpty())
        return plots.first();
    else
        return nullptr;
}
