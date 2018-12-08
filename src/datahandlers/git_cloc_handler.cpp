#include "git_cloc_handler.h"

#include "utility.h"

git_cloc_handler::git_cloc_handler()
{
}

void git_cloc_handler::addToSystemMenu(QMenu *menu, QCustomPlot *plot)
{
    Q_UNUSED(plot);
    menu->addAction( QIcon(":/graphics/genericData.png"), tr("&OpenGitClocFile"), this, SLOT(menuDataImport()) );
}

QPushButton *git_cloc_handler::addToMessageBox(QMessageBox &msgBox, QCustomPlot *plot)
{
    Q_UNUSED(plot);
    return msgBox.addButton(tr("&OpenGitClocFile"), QMessageBox::ActionRole);
}

//Can probably make this generic
void git_cloc_handler::addToContextMenu(QMenu *menu, QCustomPlot *plot)
{
    if ( metaData.isEmpty() || (!plot->selectedPlottables().empty()) )
        return;

    QVariantMap menuActionMap;
    menuActionMap["Active Plot"] = qVariantFromValue( static_cast <void *>(plot));

    QMenu *clocMenu = menu->addMenu( tr("CLOC") );
    clocMenu->installEventFilter(this);

    menuActionMap["Key Value"] = "ALL";
    menuActionMap["Data Value Storage Index"] = std::numeric_limits<int>::max();
    clocMenu->addAction( tr("All Languages"), this, SLOT(dataPlot()))->setData(menuActionMap);

    menuActionMap["Key Value"] = "NONE";
    menuActionMap["Data Value Storage Index"] = std::numeric_limits<int>::max();
    clocMenu->addAction( tr("No Languages"), this, SLOT(dataPlot()))->setData(menuActionMap);

    clocMenu->addSeparator();

    //Iterate through meta data and append menus as needed
    for (int metaDataIndex = 0; metaDataIndex < metaData.size(); metaDataIndex++) {
        menuActionMap = metaData.value(metaDataIndex);
        menuActionMap["Active Plot"] = qVariantFromValue( static_cast <void *>(plot));

        QAction *action = clocMenu->addAction(QIcon(":/graphics/visible.png"), menuActionMap.value("Key Field").toString(), this, SLOT(dataPlot()));
        action->setData(menuActionMap);

        if (menuActionMap.value("Active") == false)
            action->setIconVisibleInMenu(false);
    }
}

void git_cloc_handler::dataImport(const QVariantMap &modifier)
{
    QFileInfo fileName = QFileInfo(modifier.value("File Name").toString());
    if (fileName.exists()) {
        metaData.clear();
        if (!clocData.isOpen())
            connectToClocDatabase(&clocData);
        else {
            // Drop existing databases!
            QSqlQuery query(clocData);
            query.exec(QString("DROP TABLE snapshots"));
            query.exec(QString("DROP TABLE results"));
        }

        openSqlFile(fileName.absoluteFilePath());
    }
}

void git_cloc_handler::updateAxis(QCustomPlot *plot)
{
    Q_UNUSED(plot);
}

bool git_cloc_handler::eventFilter(QObject *object, QEvent *event)
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
                    for (int metaDataIndex = 0; metaDataIndex < metaData.size(); metaDataIndex++) {
                        QVariantMap mData = metaData.value(metaDataIndex);
                        if ( (mData.value("Key Value") == actionData.value("Key Value")) && (mData.value("Key Field") == actionData.value("Key Field")) )
                            action->setIconVisibleInMenu(mData.value("Active").toBool());
                    }
                }
                objectMenu->update();

                //Returning true prevents the standard event processing and keeps the menu open
                return true;
            }
        }
    }
    return false;
}

void git_cloc_handler::menuDataImport()
{
    if (auto *contextAction = qobject_cast<QAction *>(sender())) {
        QVariantMap modifier = contextAction->data().toMap();
        QSettings settings;

        QString fileName = QFileDialog::getOpenFileName (nullptr, tr("Open CLOC results file"),
                                                         settings.value("Git CLOC Handler Source Directory").toString(), "TEXT (*.txt);;ANY (*.*)");
        if (!fileName.isEmpty()) {
            //Remember directory
            settings.setValue("Git CLOC Handler Source Directory", QFileInfo(fileName).absolutePath());
            modifier["File Name"] = fileName;

            dataImport(modifier);
        }
    }
}

void git_cloc_handler::dataPlot()
{
    // make sure this slot is really called by a context menu action, so it carries the data we need
    if (auto *contextAction = qobject_cast<QAction *>(sender())) {
        QVariantMap selectionData = contextAction->data().toMap();
        QCustomPlot *customPlot = static_cast <QCustomPlot *>(selectionData["Active Plot"].value<void *>());

        QSharedPointer<QCPAxisTickerText> textTicker(new QCPAxisTickerText);
        QDateTime timestamp;

        for (int metaDataIndex = 0; metaDataIndex < metaData.size(); metaDataIndex++) {
            QVariantMap mData = metaData.value(metaDataIndex);

            if (selectionData.value("Key Value") == "ALL") {
                mData["Active"] = true;
            } else if (selectionData.value("Key Value") == "NONE") {
                mData["Active"] = false;
            } else if ( mData.value("Key Value") == selectionData.value("Key Value") ) {
                if (mData.value("Active") == true) {
                    mData["Active"] = false;
                } else {
                    mData["Active"] = true;
                }
            }
            metaData[metaDataIndex] = mData;
        }

        QSqlQuery query(clocData);
        bool ok;
        int numberOfVersions = 0;
        int resolution = 0x7FFFFFFF;
        double dateTime;

        QVector <double> sameLines, modifiedLines, addedLines, removedLines, dates;

        //Get number of versions
        query.exec("SELECT MAX(id) FROM snapshots");
        query.next();
        numberOfVersions = query.value(0).toInt();

        //Left justify the legend as data will probably increase left to right
        customPlot->axisRect()->insetLayout()->setInsetAlignment(0, Qt::AlignLeft | Qt::AlignTop);

        //Label Axes
        customPlot->xAxis->setLabel(tr("Date"));
        customPlot->yAxis->setLabel(tr("Lines of Code"));

        //Determine the intervals of the data
        query.exec(QString("SELECT target_timestamp FROM snapshots"));
        while (query.next()) {
            static double prevDateTime = 0;
            dateTime = query.value(0).toInt();

            if ( (prevDateTime != 0) && (qAbs(prevDateTime - dateTime) < resolution) && (qAbs(prevDateTime - dateTime) >= 86400) )
                resolution = qAbs(prevDateTime - dateTime);

            prevDateTime = dateTime;
        }

        //Round up to nearest day
        if (resolution % 86400)
            resolution += 86400 - (resolution % 86400);

        for (int version = numberOfVersions ; version > 0 ; version-- ) {
            //Get date of version
            query.exec(QString("SELECT target_timestamp FROM snapshots WHERE id = %1").arg(version));
            query.next();
            dateTime = query.value(0).toInt();

            //Fill in gaps here
            if ( (!dates.empty()) && ((dateTime - dates.last()) > (resolution * 1.1)) ) {
                while ( (dateTime - dates.last()) > (resolution * 1.1) ) {
                    dates.append(dates.last() + resolution );
                    timestamp.setTime_t(dates.last());
                    textTicker->addTick(dates.last(), timestamp.toString("dd. MMMM yyyy"));

                    sameLines.append(sameLines.last() + modifiedLines.last() + addedLines.last());

                    modifiedLines.append(0);
                    addedLines.append(0);
                    removedLines.append(0);
                }
            }

            //Add date stamp to the dates vector
            dates.append(dateTime);
            timestamp.setTime_t(dates.last());
            textTicker->addTick(dates.last(), timestamp.toString("dd. MMMM yyyy"));

            sameLines.append(0);
            modifiedLines.append(0);
            addedLines.append(0);
            removedLines.append(0);

            for (const auto& keyFieldMetaData : metaData) {
                if (keyFieldMetaData.value("Active") == true) {
                    query.exec(QString("SELECT nCode FROM results WHERE snapshot_id=%1 AND type='count' AND language='%2'").arg(version).arg(keyFieldMetaData.value("Key Field").toString()));
                    query.next();
                    if (query.isValid())
                        sameLines.last() = sameLines.last() + query.value(0).toInt(&ok);

                    query.exec(QString("SELECT nCode FROM results WHERE snapshot_id=%1 AND type='modified' AND language='%2'").arg(version).arg(keyFieldMetaData.value("Key Field").toString()));
                    query.next();
                    if (query.isValid())
                        modifiedLines.last() = modifiedLines.last() + query.value(0).toInt(&ok);

                    query.exec(QString("SELECT nCode FROM results WHERE snapshot_id=%1 AND type='removed' AND language='%2'").arg(version).arg(keyFieldMetaData.value("Key Field").toString()));
                    query.next();
                    if (query.isValid())
                        removedLines.last() = removedLines.last() - query.value(0).toInt(&ok);

                    query.exec(QString("SELECT nCode FROM results WHERE snapshot_id=%1 AND type='added' AND language='%2'").arg(version).arg(keyFieldMetaData.value("Key Field").toString()));
                    query.next();
                    if (query.isValid())
                        addedLines.last() = addedLines.last() + query.value(0).toInt(&ok);
                }
            }
        }

        customPlot->clearGraphs();
        customPlot->clearPlottables();
        customPlot->clearItems();

        auto *same = new QCPBars(customPlot->xAxis, customPlot->yAxis);
        auto *modified = new QCPBars(customPlot->xAxis, customPlot->yAxis);
        auto *added = new QCPBars(customPlot->xAxis, customPlot->yAxis);
        auto *removed = new QCPBars(customPlot->xAxis, customPlot->yAxis);

        removed->setName(tr("Removed"));
        removed->setData(dates, removedLines);
        removed->setBrush(Qt::darkGray);
        removed->setWidth(resolution);

        same->setName(tr("Same"));
        same->setData(dates, sameLines);
        same->setBrush(Qt::green);
        same->setWidth(resolution);

        modified->setName(tr("Modified"));
        modified->setData(dates, modifiedLines);
        modified->setBrush(Qt::yellow);
        modified->setWidth(resolution);

        added->setName(tr("Added"));
        added->setData(dates, addedLines);
        added->setBrush(Qt::red);
        added->setWidth(resolution);

        //Stack Bars
        added->moveAbove(modified);

        //TODO: Reduce tick labels on large runs
        customPlot->xAxis->setTicker(textTicker);
        customPlot->xAxis->setTickLabelRotation(30);

        customPlot->plottable()->rescaleAxes();
        for (int plotIndex = 0; plotIndex < customPlot->plottableCount(); plotIndex++) {
            customPlot->plottable(plotIndex)->rescaleAxes(true);
        }
        customPlot->replot();
        //ah.updateGraphAxes(customPlot);
    }
}

void git_cloc_handler::dataExport(QVariantMap modifier)
{
    Q_UNUSED(modifier);
}

void git_cloc_handler::openSqlFile(const QString &fileName)
{
    QFile file(fileName);
    QString line = QString();

    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream textStream(&file);
        textStream.setAutoDetectUnicode(true);

        //Open connection to database
        QSqlQuery query(clocData);

        while (!textStream.atEnd()) {
            line = QString(textStream.readLine());

            if (!line.isEmpty() && !line.startsWith(QChar::Null)) {
                query.exec(line);
            }
        }
        file.close();

        //TODO: Treat Languages like ("Unique Event Meta Data")?
        query.exec("SELECT DISTINCT(language) FROM results");
        while (query.next()) {
            if (query.value(0).toString() == "SUM")
                continue;

            QVariantMap variantMap;
            variantMap["Key Field"] = query.value(0).toString();
            variantMap["Key Value"] = query.value(0).toString();
            variantMap["Active"] = false;
            metaData.append(variantMap);
        }
    }
}

void git_cloc_handler::connectToClocDatabase(QSqlDatabase *clocData)
{
    //Create blank database
    *clocData = QSqlDatabase::addDatabase("QSQLITE", "clocData");
    //Store the data in RAM
    clocData->setDatabaseName(":memory:");
    //Make the connection
    clocData->open();
}
