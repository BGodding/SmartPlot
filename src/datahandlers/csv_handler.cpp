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

void csv_handler::addToSystemMenu(QMenu *menu, QCustomPlot *plot)
{
    QVariantMap menuActionMap;
    menuActionMap["Active Plot"] = qVariantFromValue( static_cast <void *>(plot));
    QAction *action = menu->addAction( QIcon(":/graphics/genericData.png"), tr("&OpenDataFile"), this, SLOT(menuDataImport()) );
    action->setData(menuActionMap);
    action->setStatusTip(tr("Use this option to open basic delimited text files"));
    action->setWhatsThis(tr("Use this option to open basic delimited text files"));
}

QPushButton *csv_handler::addToMessageBox(QMessageBox &msgBox, QCustomPlot *plot)
{
    Q_UNUSED(plot);
    return msgBox.addButton(tr("&OpenDataFile"), QMessageBox::ActionRole);
}

//Can probably make this generic
void csv_handler::addToContextMenu(QMenu *menu, QCustomPlot *plot)
{
    if ( metaData.isEmpty() || (!plot->selectedPlottables().empty()) )
        return;

    QAction *eventAction;
    QMenu *csvHandlerMenu = menu->addMenu( tr("Generic") );
    QMenu *eventsMenu = csvHandlerMenu->addMenu( tr("CSV Events") );
#ifndef MOBILE
    csvHandlerMenu->installEventFilter(this);
#endif

    QVariant currentKeyField;
    //Iterate through meta data and append menus as needed
    for (int metaDataIndex = 0; metaDataIndex < metaData.size(); metaDataIndex++) {
        //TODO: Just make a menu action map entry?
        QVariantMap menuActionMap = metaData.value(metaDataIndex);
        menuActionMap.remove("Unique Event Meta Data");
        menuActionMap["Active Plot"] = qVariantFromValue( static_cast <void *>(plot));

        if (menuActionMap.value("Data Type") == "Series") {
            eventAction = csvHandlerMenu->addAction(menuActionMap.value("Key Field").toString(), this, SLOT(dataPlot()));
            eventAction->setData(menuActionMap);
        } else if (menuActionMap.value("Data Type") == "Event") {
            if (eventsMenu->isEmpty()) {
                eventsMenu->installEventFilter(this);

                menuActionMap["Key Value"] = "****";
                eventsMenu->addAction( tr("All Events"), this, SLOT(dataPlot()))->setData(menuActionMap);

                menuActionMap["Key Value"] = "    ";
                eventsMenu->addAction( tr("No Events"), this, SLOT(dataPlot()))->setData(menuActionMap);
            }

            QList<QVariant> uniqueEventMetaData = metaData.at(metaDataIndex).value("Unique Event Meta Data").toList();
            for (int uniqueEventMetaDataIndex = 0; uniqueEventMetaDataIndex < uniqueEventMetaData.size() ; uniqueEventMetaDataIndex++) {
                QVariantMap uniqueEventMetaDataMap = uniqueEventMetaData.value(uniqueEventMetaDataIndex).toMap();

                if (uniqueEventMetaDataMap.value("Key Value").toString().isEmpty())
                    continue;

                //If the key field changes we need to create a new header in the menu
                if (currentKeyField != uniqueEventMetaDataMap.value("Key Field")) {
                    menuActionMap.remove("Key Value");

                    QAction *action = eventsMenu->addAction(menuActionMap.value("Key Field").toString(), this, SLOT(dataPlot()));

                    //Make the header menu items stand out
                    QFont actionFont = action->font();
                    actionFont.setBold(true);
                    actionFont.setUnderline(true);
                    action->setFont(actionFont);

                    action->setData(menuActionMap);

                    currentKeyField = uniqueEventMetaDataMap.value("Key Field");
                }

                menuActionMap["Key Value"] = uniqueEventMetaDataMap.value("Key Value");
                QAction *action = eventsMenu->addAction(QIcon(":/graphics/visible.png"), menuActionMap.value("Key Value").toString(), this, SLOT(dataPlot()));
                action->setData(menuActionMap);

                if (uniqueEventMetaDataMap.value("Active") == false)
                    action->setIconVisibleInMenu(false);
            }
            eventsMenu->addSeparator();
            scaleMenuForScreen(eventsMenu, plot);
        }
    }
}

void csv_handler::dataImport(const QVariantMap &modifier)
{
    QFileInfo fileName = QFileInfo(modifier.value("File Name").toString());
    if (fileName.exists()) {
        if (!metaData.isEmpty()) {
            if ( QMessageBox::question(nullptr, tr("File Already Open"), tr("Add data to existing dataset?")) == QMessageBox::No) {
                seriesData.clear();
                eventData.clear();
                tickLabelLookup.clear();
                metaData.clear();
            }
        }

        openDelimitedFile(fileName.absoluteFilePath());

        generateUniqueLists(&eventData, metaData);

        QCustomPlot *plot = (QCustomPlot *)modifier.value("Active Plot").value<void *>();
        ah.setAxisType(plot->xAxis, axis_handler::fixed);
    }
}

void csv_handler::updateAxis(QCustomPlot *plot)
{
    ah.updateAxis(plot, metaData, eventData, tickLabelLookup, seriesData.first(), plot->xAxis2, maxVerticalEvents);
}

bool csv_handler::eventFilter(QObject *object, QEvent *event)
{
    if ( event->type() == QEvent::MouseButtonRelease ) {
        auto *objectMenu = qobject_cast<QMenu *>(object);

        //Check if object is actually a menu
        if (objectMenu != nullptr) {
            QAction *menuAction = objectMenu->activeAction();

            //Check if the selected item has an action
            if (menuAction != nullptr) {
                objectMenu->activeAction()->trigger();

                //The events menu has icons that will need to be updated after action is triggered
                foreach (QAction *action, objectMenu->actions()) {
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
    if (auto *contextAction = qobject_cast<QAction *>(sender())) {
        QVariantMap modifier = contextAction->data().toMap();
        QSettings settings;

        QString fileName = QFileDialog::getOpenFileName (nullptr, tr("Open Delimited Text file"),
                                                         settings.value("CSV Handler Source Directory").toString(), "TEXT (*.txt);;CSV (*.csv);;ANY (*.*)");
        if (!fileName.isEmpty()) {
            //Remember directory
            settings.setValue("CSV Handler Source Directory", QFileInfo(fileName).absolutePath());
            modifier["File Name"] = fileName;

            dataImport(modifier);
        }
    }
}

void csv_handler::dataPlot()
{
    // make sure this slot is really called by a context menu action, so it carries the data we need
    if (auto *contextAction = qobject_cast<QAction *>(sender())) {
        QVariantMap selectionData = contextAction->data().toMap();
        QCustomPlot *plot = static_cast <QCustomPlot *>(selectionData["Active Plot"].value<void *>());

        if (selectionData.value("Data Type") == "Series") {
            ph.addPlotLine( seriesData, selectionData );
            ah.updateGraphAxes(plot);
            plot->replot();
        } else if (selectionData.value("Data Type") == "Event") {
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

void csv_handler::openDelimitedFile(const QString &fileName)
{
    int estimatedLineCount = th.estimateLineCount(fileName);

    QFile file(fileName);
    QString line = QString();
    QString delimiter;
    int firstDataColumn = 0;
    int startingMetaDatalistIndex = metaData.size();
    int metaDatalistIndex = startingMetaDatalistIndex;

    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream textStream(&file);
        textStream.setAutoDetectUnicode(true);

        QProgressDialog progress(tr("Importing Data"), tr("Cancel"), 0, int(file.size()), nullptr, (Qt::WindowCloseButtonHint | Qt::WindowStaysOnTopHint));
        progress.setWindowModality(Qt::WindowModal);
        progress.setMinimumDuration(0);

        // Determine the delimiter
        delimiter = th.autoDetectDelimiter(textStream);

        // Check if the first column looks like a key axis (all values increasing)
        if (th.firstColumnIsIncrimental(textStream, delimiter)) {
            if ( QMessageBox::question(nullptr, tr("Key Axis Found"), tr("Use first column as key axis?")) == QMessageBox::Yes)
                firstDataColumn = 1;
        }

        // This function grabs the first line of the file and uses it for naming data columns
        th.checkAndProcessColumnHeaders( textStream, delimiter, metaData, firstDataColumn );

        // The data keys are always store in the series data
        int dataKeyColumn = seriesData.size();

        while (!textStream.atEnd() && !progress.wasCanceled()) {
            line = QString(textStream.readLine());

            //Refreshing the bar after every line is excessive
            if (estimatedLineCount % 1000 == 0)
                progress.setValue(int(file.pos()));

            // Throw away lines with no useful data
            if (!line.isEmpty() && !line.startsWith(QChar::Null)) {
                // Throw away lines if the file appears to be larger than the max allowed size
                if ( maxPlotSize < estimatedLineCount-- )
                    continue;

                if (metaDatalistIndex != metaData.size()) {
                    QStringList strings = line.split(delimiter);
                    bool ok;

                    //Add column for date/time
                    QVector<double> tempVector;
                    seriesData.append(tempVector);

                    for (; metaDatalistIndex < metaData.size(); metaDatalistIndex++) {
                        QVariantMap &mData = metaData[metaDatalistIndex];

                        QString targetString = strings.value(mData.value("Data Source").toInt());
                        // Here we make the assumption that if the first value for the column is
                        // non-numerical the column contains events (string data)
                        (void)targetString.toDouble(&ok);
                        if (ok) {
                            QVector<double> tempVector;
                            seriesData.append(tempVector);
                            mData.insert( "Data Type", "Series" );
                            mData.insert( "Data Value Storage Index", (seriesData.size() - 1) );
                            mData.insert( "Data Key Storage Index", dataKeyColumn );
                        } else {
                            QVector<QString> tempVector;
                            eventData.append(tempVector);
                            mData.insert( "Data Type", "Event" );
                            mData.insert( "Data Value Storage Index", (eventData.size() - 1) );
                            mData.insert( "Data Key Storage Index", dataKeyColumn );
                            mData.insert( "Action", "OR");
                            mData.insert( "Tick Label", true);
                        }
                    }
                }

                processLineFromFile(line, delimiter, dataKeyColumn, startingMetaDatalistIndex, metaData );
            }
        }
        file.close();
    }
}

void csv_handler::processLineFromFile(const QString &line, const QString &delimiter, int dataKeyColumn, int metaDataIndexStart, QList<QVariantMap> &metaData)
{
    QStringList strings = line.split(delimiter);

    //If the first data column is 0 use size as the x-axis
    if (metaData.first().value("Data Source").toInt() == 0) {
        seriesData[dataKeyColumn].append(seriesData[dataKeyColumn].size());
    } else {
        seriesData[dataKeyColumn].append(strings.first().toInt());
    }

    //Iterate through meta data and append data as needed
    for (int metaDataIndex = metaDataIndexStart; metaDataIndex < metaData.size(); metaDataIndex++) {
        const QVariantMap &metaDataAtIndex = metaData.at(metaDataIndex);

        if (metaDataAtIndex.contains("Data Type")) {
            if (metaDataAtIndex.value("Data Type") == "Series") {
                QString string = strings.value(metaDataAtIndex.value("Data Source").toInt());
                double value;
                bool ok;
                value = string.toDouble(&ok);

                if (!ok)
                    value = std::numeric_limits<double>::quiet_NaN();

                seriesData[metaDataAtIndex.value("Data Value Storage Index").toInt()].append(value);
            } else if (metaDataAtIndex.value("Data Type") == "Event") {
                QString string = strings.value(metaDataAtIndex.value("Data Source").toInt());
                eventData[metaDataAtIndex.value("Data Value Storage Index").toInt()].append(string);
            }
        }
    }
}
