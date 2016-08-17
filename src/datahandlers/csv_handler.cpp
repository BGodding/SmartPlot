#include "csv_handler.h"

#include "utility.h"

csv_handler::csv_handler()
{
#ifdef MOBILE
    maxVerticalEvents = 2;
    maxPlotSize = 10000;
    dataDeadTime = 600;
#else
    maxVerticalEvents = 4;
    maxPlotSize = 500000;
    dataDeadTime = 600;
#endif
}

void csv_handler::addToSystemMenu(QMenu *menu)
{
    QAction * action = menu->addAction( QIcon(":/graphics/genericData.png"), tr("&OpenDataFile"), this, SLOT(menuDataImport()) );
    action->setStatusTip(tr("Use this option to open basic delimited text files"));
    action->setWhatsThis(tr("Use this option to open basic delimited text files"));
}

QPushButton *csv_handler::addToMessageBox(QMessageBox &msgBox)
{
    return msgBox.addButton(tr("&OpenDataFile"), QMessageBox::ActionRole);
}

//Can probably make this generic
void csv_handler::addToContextMenu(QMenu *menu, QCustomPlot* plot)
{
    if( metaData.isEmpty() || (plot->selectedPlottables().size() > 0) )
        return;

    QVariantMap menuActionMap;
    QAction *eventAction;
    QMenu *csvHandlerMenu = menu->addMenu( tr("Generic") );
    QMenu *eventsMenu = csvHandlerMenu->addMenu( tr("CSV Events") );
    #ifndef MOBILE
    csvHandlerMenu->installEventFilter(this);
    #endif

    QVariant currentKeyField;
    //Iterate through meta data and append menus as needed
    for(int metaDataIndex = 0; metaDataIndex < metaData.size(); metaDataIndex++)
    {
        QVariantMap keyFieldMap = metaData.value(metaDataIndex);

        menuActionMap = keyFieldMap;
        menuActionMap.remove("Unique Event Meta Data");
        menuActionMap["Active Plot"] = qVariantFromValue( (void *)plot);

        if(menuActionMap.value("Value Data Type") == "Series")
        {
            eventAction = csvHandlerMenu->addAction(menuActionMap.value("Key Field").toString(), this, SLOT(dataPlot()));
            eventAction->setData(menuActionMap);
        }
        else if(menuActionMap.value("Value Data Type") == "Event")
        {
            if(eventsMenu->isEmpty())
            {
                eventsMenu->installEventFilter(this);

                menuActionMap["Key Value"] ="****";
                eventsMenu->addAction( tr("All Events"), this, SLOT(dataPlot()))->setData(menuActionMap);

                menuActionMap["Key Value"] ="    ";
                eventsMenu->addAction( tr("No Events"), this, SLOT(dataPlot()))->setData(menuActionMap);
            }

            QList<QVariant> uniqueEventMetaData = keyFieldMap.value("Unique Event Meta Data").toList();
            for(int uniqueEventMetaDataIndex = 0; uniqueEventMetaDataIndex < uniqueEventMetaData.size() ; uniqueEventMetaDataIndex++)
            {
                QVariantMap uniqueEventMetaDataMap = uniqueEventMetaData.value(uniqueEventMetaDataIndex).toMap();

                if(uniqueEventMetaDataMap.value("Key Value").toString().isEmpty())
                    continue;

                if(currentKeyField != uniqueEventMetaDataMap.value("Key Field"))
                {
                    QVariantMap menuActionMapHeader = menuActionMap;
                    menuActionMapHeader.remove("Key Value");

                    QAction* action = eventsMenu->addAction(menuActionMapHeader.value("Key Field").toString(), this, SLOT(dataPlot()));

                    //Make the header menu items stand out
                    QFont actionFont = action->font();
                    actionFont.setBold(true);
                    actionFont.setUnderline(true);
                    action->setFont(actionFont);

                    action->setData(menuActionMapHeader);

                    currentKeyField = uniqueEventMetaDataMap.value("Key Field");
                }

                menuActionMap["Key Value"] = uniqueEventMetaDataMap.value("Key Value");
                QAction* action = eventsMenu->addAction(QIcon(":/graphics/visible.png"), menuActionMap.value("Key Value").toString(), this, SLOT(dataPlot()));
                action->setData(menuActionMap);

                if(uniqueEventMetaDataMap.value("Active") == false)
                    action->setIconVisibleInMenu(false);
            }
            eventsMenu->addSeparator();
            scaleMenuForScreen(eventsMenu, plot);
        }
    }
}

void csv_handler::dataImport(QVariantMap modifier)
{
    QFileInfo fileName = QFileInfo(modifier.value("File Name").toString());
    if(fileName.exists())
    {
        seriesData.clear();
        eventData.clear();
        tickLabelLookup.clear();
        metaData.clear();

        openDelimitedFile(fileName.absoluteFilePath());

        generateUniqueLists(&eventData, metaData);
    }
}

void csv_handler::updateAxis(QCustomPlot *plot)
{
    ah.updateAxis(plot, metaData, eventData, tickLabelLookup, seriesData.first(), plot->xAxis2, maxVerticalEvents);
}

bool csv_handler::eventFilter(QObject* object,QEvent* event)
{
    if ( event->type() == QEvent::MouseButtonRelease )
    {
        QMenu *objectMenu = qobject_cast<QMenu*>(object);

        //Check if object is actually a menu
        if(objectMenu != NULL)
        {
            QAction *menuAction = objectMenu->activeAction();

            //Check if the selected item has an action
            if(menuAction != NULL)
            {
                objectMenu->activeAction()->trigger();

                //The events menu has icons that will need to be updated after action is triggered
                foreach(QAction* action, objectMenu->actions())
                {
                    QVariantMap actionData = action->data().toMap();

                    action->setIconVisibleInMenu(ah.isActionVisible(actionData, metaData));
                }
                objectMenu->update();

                //Returning true prevents the standard event processing and keeps the menu open
                return true;
            }
        }
    }
    return false;
}

void csv_handler::menuDataImport()
{
    QSettings settings;
    QVariantMap modifier;

    QString fileName = QFileDialog::getOpenFileName (NULL, tr("Open Delimited Text file"),
                                                     settings.value("CSV Handler Source Directory").toString(), "TEXT (*.txt);;CSV (*.csv);;ANY (*.*)");
    if(!fileName.isEmpty())
    {
        //Remember directory
        settings.setValue("CSV Handler Source Directory", QFileInfo(fileName).absolutePath());
        modifier["File Name"] = fileName;

        dataImport(modifier);
    }
}

void csv_handler::dataPlot()
{
    // make sure this slot is really called by a context menu action, so it carries the data we need
    if (QAction* contextAction = qobject_cast<QAction*>(sender()))
    {
        QVariantMap selectionData = contextAction->data().toMap();
        QCustomPlot* plot = (QCustomPlot*)selectionData["Active Plot"].value<void *>();

        if(selectionData.value("Value Data Type") == "Series")
        {
            ph.addPlotLine( seriesData, selectionData );
            ah.updateGraphAxes(plot);
            plot->replot();
        }
        else if (selectionData.value("Value Data Type") == "Event")
        {
            tickLabelLookup.clear();

            //Update the events with what the user choose
            ah.toggleKeyValueVisibleInList(selectionData, metaData);
            ah.toggleKeyFieldVisibleInList(selectionData, metaData);
        }
        updateAxis(plot);
    }
}

void csv_handler::dataExport(QVariantMap modifier)
{
    Q_UNUSED(modifier);
}

void csv_handler::openDelimitedFile(QString fileName)
{
    int estimatedLineCount = th.estimateLineCount(fileName);

    QFile file(fileName);
    QString line = QString();
    QString delimiter;
    int firstDataColumn = 0;

    if (file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        QTextStream textStream(&file);
        textStream.setAutoDetectUnicode(true);

        QProgressDialog progress(tr("Importing Data"), tr("Cancel"), 0, file.size(), NULL, (Qt::WindowCloseButtonHint | Qt::WindowStaysOnTopHint));
        progress.setWindowModality(Qt::WindowModal);
        progress.setMinimumDuration(0);

        //we need to determine the delimiter
        delimiter = th.autoDetectDelimiter(textStream);

        if(th.firstColumnIsIncrimental(textStream, delimiter))
        {
            if( QMessageBox::question(NULL, tr("Key Axis Found"), tr("Use first column as key axis?")) == QMessageBox::Yes)
                firstDataColumn = 1;
        }

        th.checkAndProcessColumnHeaders( textStream, delimiter, metaData, firstDataColumn );

        while (!textStream.atEnd() && !progress.wasCanceled())
        {
            line = QString(textStream.readLine());
            progress.setValue(file.pos());

            if (!line.isEmpty() && !line.startsWith(QChar::Null))
            {
                if( maxPlotSize < estimatedLineCount-- )
                    continue;

                //Init data storage here
                if(seriesData.isEmpty() && eventData.isEmpty())
                {
                    QStringList strings = line.split(delimiter);
                    bool ok;

                    //Add column for date/time
                    if(firstDataColumn == 0)
                    {
                        QVector<double> tempVector;
                        seriesData.append(tempVector);
                    }

                    for(int listIndex = 0;listIndex < metaData.size();listIndex++)
                    {
                        QVariantMap& mData = metaData[listIndex];

                        QString targetString = strings.value(mData.value("Data Source").toInt());
                        (void)targetString.toDouble(&ok);
                        if(ok)
                        {
                            QVector<double> tempVector;
                            seriesData.append(tempVector);
                            mData.insert( "Value Data Type", "Series" );
                            mData.insert( "Data Storage", (seriesData.size() - 1) );
                        }
                        else
                        {
                            QVector<QString> tempVector;
                            eventData.append(tempVector);
                            mData.insert( "Value Data Type", "Event" );
                            mData.insert( "Data Storage", (eventData.size() - 1) );
                            mData.insert( "Action", "OR");
                            mData.insert( "Tick Label", true);
                        }
                    }
                }

                processLineFromFile(line, delimiter, metaData );
            }
        }
        file.close();
    }
}

void csv_handler::processLineFromFile(QString line, QString delimiter, QList<QVariantMap> &metaData)
{
    QStringList strings = line.split(delimiter);

    //If the first data column is 0 use size as the x-axis
    if(metaData.first().value("Data Source").toInt() == 0)
    {
        seriesData[0].append(seriesData[0].size());
    }

    //Iterate through meta data and append data as needed
    for(int metaDataIndex = 0; metaDataIndex < metaData.size(); metaDataIndex++)
    {
        QVariantMap& mData = metaData[metaDataIndex];

        if(mData.contains("Value Data Type"))
        {
            if(mData.value("Value Data Type") == "Series")
            {
                QString string = strings.value(mData.value("Data Source").toInt());
                double value;
                bool ok;
                value = string.toDouble(&ok);

                if(!ok)
                    value = std::numeric_limits<double>::quiet_NaN();

                seriesData[mData.value("Data Storage").toInt()].append(value);
            }
            else if(mData.value("Value Data Type") == "Event")
            {
                QString string = strings.value(mData.value("Data Source").toInt());
                eventData[mData.value("Data Storage").toInt()].append(string);
            }
        }
    }
}
