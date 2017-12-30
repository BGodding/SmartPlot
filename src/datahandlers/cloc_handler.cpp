#include "cloc_handler.h"

#include "utility.h"

cloc_handler::cloc_handler()
{
}

void cloc_handler::addToSystemMenu(QMenu *menu)
{
    QAction * action = menu->addAction( QIcon(":/graphics/genericData.png"), tr("&OpenClocFile"), this, SLOT(menuDataImport()) );
}

QPushButton *cloc_handler::addToMessageBox(QMessageBox &msgBox)
{
    return msgBox.addButton(tr("&OpenClocFile"), QMessageBox::ActionRole);
}

//Can probably make this generic
void cloc_handler::addToContextMenu(QMenu *menu, QCustomPlot* plot)
{
    if( metaData.isEmpty() || (plot->selectedPlottables().size() > 0) )
        return;

    QVariantMap menuActionMap;
    QAction *eventAction;
    QMenu *clocHandlerMenu = menu->addMenu( tr("CLOC") );
    #ifndef MOBILE
    clocHandlerMenu->installEventFilter(this);
    #endif

//    QVariant currentKeyField;
//    //Iterate through meta data and append menus as needed
//    for(int metaDataIndex = 0; metaDataIndex < metaData.size(); metaDataIndex++)
//    {
//        QVariantMap keyFieldMap = metaData.value(metaDataIndex);

//        menuActionMap = keyFieldMap;
//        menuActionMap.remove("Unique Event Meta Data");
//        menuActionMap["Active Plot"] = qVariantFromValue( (void *)plot);

//        if(menuActionMap.value("Value Data Type") == "Series")
//        {
//            eventAction = clocHandlerMenu->addAction(menuActionMap.value("Key Field").toString(), this, SLOT(dataPlot()));
//            eventAction->setData(menuActionMap);
//        }
//    }
}

void cloc_handler::dataImport(QVariantMap modifier)
{
    QFileInfo fileName = QFileInfo(modifier.value("File Name").toString());
    if(fileName.exists())
    {
        seriesData.clear();
        metaData.clear();
        connectToClocDatabase(&clocData);

        openDelimitedFile(fileName.absoluteFilePath());
    }
}

void cloc_handler::updateAxis(QCustomPlot *plot)
{
}

bool cloc_handler::eventFilter(QObject* object,QEvent* event)
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

void cloc_handler::menuDataImport()
{
    QSettings settings;
    QVariantMap modifier;

    QString fileName = QFileDialog::getOpenFileName (NULL, tr("Open Delimited Text file"),
                                                     settings.value("CLOC Handler Source Directory").toString(), "TEXT (*.txt);;ANY (*.*)");
    if(!fileName.isEmpty())
    {
        //Remember directory
        settings.setValue("CLOC Handler Source Directory", QFileInfo(fileName).absolutePath());
        modifier["File Name"] = fileName;

        dataImport(modifier);
    }
}

void cloc_handler::dataPlot()
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
        updateAxis(plot);
    }
}

void cloc_handler::dataExport(QVariantMap modifier)
{
    Q_UNUSED(modifier);
}

void cloc_handler::openDelimitedFile(QString fileName)
{
    //This will now be imporint a file with SQL commands, and executing them as is..
    QFile file(fileName);
    QString line = QString();

    if (file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        QTextStream textStream(&file);
        textStream.setAutoDetectUnicode(true);

        QProgressDialog progress(tr("Importing Data"), tr("Cancel"), 0, file.size(), NULL, (Qt::WindowCloseButtonHint | Qt::WindowStaysOnTopHint));
        progress.setWindowModality(Qt::WindowModal);
        progress.setMinimumDuration(0);

        //Open connection to database
        QSqlQuery query(clocData);
        //QString sqlCommand;
        //bool ok;

        while (!textStream.atEnd() && !progress.wasCanceled())
        {
            line = QString(textStream.readLine());
            progress.setValue(file.pos());

            if (!line.isEmpty() && !line.startsWith(QChar::Null))
            {
                qDebug() << line;
                query.exec(line);
                query.next();
                qDebug() << query.lastError();
            }
        }
        file.close();

        //Get number of versions
        query.exec("SELECT MAX(id) FROM results");
        query.next();
        int numberOfVersions = query.value(0).toInt();
        qDebug() << numberOfVersions;
    }
}

void cloc_handler::connectToClocDatabase(QSqlDatabase *clocData)
{
    //Create blank database
    *clocData = QSqlDatabase::addDatabase("QSQLITE", "clocData");
    //Store the data in RAM
    clocData->setDatabaseName(":memory:");
    //Make the connection
    clocData->open();
}
