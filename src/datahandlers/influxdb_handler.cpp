#include "influxdb_handler.h"

#include <QInputDialog>
#include <QSettings>
#include <utility>

void influxdb_handler::setInfluxAddress(QUrl url)
{
    influxAddress = url;
}

influxdb_handler::influxdb_handler()
{
    QSettings settings;
    influxAddress = settings.value("Influx DB Host").toString();
}

void influxdb_handler::addToSystemMenu(QMenu *menu, QCustomPlot *plot)
{
    Q_UNUSED(plot);
    QMenu *influxdbHandlerMenu = menu->addMenu( tr("Influx DB") );

    influxdbHandlerMenu->addAction( QIcon(":/graphics/cloudRefresh.png"), tr("&Refresh"), this, SLOT(menuRefresh()) );
    influxdbHandlerMenu->addAction( QIcon(":/graphics/cloud.png"), tr("&Configure"), this, SLOT(menuConfigure()) );
}

void influxdb_handler::addToContextMenu(QMenu *menu, QCustomPlot *plot)
{
    if ( contextMenu.isEmpty() || (!plot->selectedPlottables().empty()) )
        return;

    influxPlot = plot;
    menu->addMenu(&contextMenu);
}

void influxdb_handler::close()
{
    qDebug() << queryStats;
}

void influxdb_handler::menuRefresh()
{
    contextMenu.clear();
    generateMetaData();
//    QJsonDocument result = query(QUrlQuery(QString("db=awi_properties&epoch=s&q=SELECT (value) FROM current_gun_1 WHERE device_id='3'")));
//    jsonDumpDoc(result);
//    QVector<double> key;
//    QVector<double> value;
//    getValues(result, key, value);
//    qDebug() << key;
//    qDebug() << value;
}

void influxdb_handler::menuConfigure()
{
    bool ok;
    QSettings settings;
    QString userString = QInputDialog::getText(nullptr, tr("SmartPlot"), "Enter Address and port", QLineEdit::Normal, settings.value("Influx DB Host").toString(), &ok,
                                               (Qt::WindowCloseButtonHint | Qt::WindowStaysOnTopHint) );

    settings.setValue("Influx DB Host", userString);
    setInfluxAddress(userString);
}

void influxdb_handler::dataPlot()
{
    // make sure this slot is really called by a context menu action, so it carries the data we need
    if (auto *contextAction = qobject_cast<QAction *>(sender())) {
        QVariantMap selectionData = contextAction->data().toMap();
        QCustomPlot *plot = static_cast <QCustomPlot *>(selectionData["Active Plot"].value<void *>());
        qDebug() << selectionData;

        QJsonDocument result = query(QUrlQuery(QString("db=%1&epoch=s&q=SELECT (%2) FROM %3 WHERE %4=\'%5\'")
                                               .arg(selectionData.value("Database").toString())
                                               .arg(selectionData.value("Field Key").toString())
                                               .arg(selectionData.value("Measurement").toString())
                                               .arg(selectionData.value("Tag Key").toString())
                                               .arg(selectionData.value("Tag Value").toString()) ));
//         qDebug() << QString("db=%1&epoch=s&q=SELECT (%2) FROM %3 WHERE %4=\'%5\'")
//                     .arg(selectionData.value(Database"].toString())
//                     .arg(selectionData.value(Field Key"].toString())
//                     .arg(selectionData.value(Measurement"].toString())
//                     .arg(selectionData.value(Tag Key"].toString())
//                     .arg(selectionData.value(Tag Value"].toString());
//         jsonDumpDoc(result);

        QVector<double> key;
        QVector<double> value;
        jh.getValues(result, key, value);
        ph.addPlotLine(key, value, selectionData.value("Measurement").toString(), influxPlot);
        ah.updateGraphAxes(plot);
        plot->replot();
    }
}

void influxdb_handler::generateMetaData()
{
    if (!contextMenu.isEmpty())
        return;

    contextMenu.setTitle( tr("Influx DB") );

    QJsonDocument databases    = query(QUrlQuery("q=SHOW DATABASES"));
    QList<QVariant> databases_list = jh.getValues(databases);
    QList<QVariant>::iterator databases_list_iterator;

    QVariantMap mData;

    for (databases_list_iterator = databases_list.begin(); databases_list_iterator != databases_list.end(); ++databases_list_iterator) {
        mData["Database"] = (*databases_list_iterator).toString();

        QJsonDocument measurements = query(QUrlQuery(QString("db=%1&q=SHOW MEASUREMENTS")
                                                     .arg((*databases_list_iterator).toString())));
        QList<QVariant> measurements_list = jh.getValues(measurements);
        QList<QVariant>::iterator measurements_list_iterator;

        if (measurements_list.isEmpty())
            continue;

        QMenu *databasesMenu = contextMenu.addMenu((*databases_list_iterator).toString());

        for (measurements_list_iterator = measurements_list.begin(); measurements_list_iterator != measurements_list.end(); ++measurements_list_iterator) {
            mData["Measurement"] = (*measurements_list_iterator).toString();

            QMenu *measurementsMenu = databasesMenu->addMenu((*measurements_list_iterator).toString());

            QJsonDocument field_keys = query(QUrlQuery(QString("db=%1&q=SHOW FIELD KEYS FROM \"%2\"")
                                                       .arg((*databases_list_iterator).toString())
                                                       .arg((*measurements_list_iterator).toString()) ));
            QList<QVariant> field_keys_list = jh.getValues(field_keys);
            QList<QVariant>::iterator field_keys_list_iterator;

            QJsonDocument tag_keys = query(QUrlQuery(QString("db=%1&q=SHOW TAG KEYS FROM \"%2\"")
                                                     .arg((*databases_list_iterator).toString())
                                                     .arg((*measurements_list_iterator).toString()) ));
            QList<QVariant> tag_keys_list = jh.getValues(tag_keys);
            QList<QVariant>::iterator tag_keys_list_iterator;

            for (field_keys_list_iterator = field_keys_list.begin(); field_keys_list_iterator != field_keys_list.end(); ++field_keys_list_iterator) {
                mData["Field Key"] = (*field_keys_list_iterator).toString();

                QMenu *fieldKeysMenu = measurementsMenu->addMenu((*field_keys_list_iterator).toString());

                for (tag_keys_list_iterator = tag_keys_list.begin(); tag_keys_list_iterator != tag_keys_list.end(); ++tag_keys_list_iterator) {
                    mData["Tag Key"] = (*tag_keys_list_iterator).toString();

                    QMenu *tagKeysMenu = fieldKeysMenu->addMenu((*tag_keys_list_iterator).toString());
                    tagKeysMenu->installEventFilter(this);

                    QJsonDocument tag_values = query(QUrlQuery(QString("db=%1&q=SHOW TAG VALUES FROM \"%2\" WITH KEY=\"%3\"")
                                                               .arg((*databases_list_iterator).toString())
                                                               .arg((*measurements_list_iterator).toString())
                                                               .arg((*tag_keys_list_iterator).toString()) ));
                    QList<QVariant> tag_values_list = jh.getValues(tag_values);
                    QList<QVariant>::iterator tag_values_list_iterator;

                    for (tag_values_list_iterator = tag_values_list.begin(); tag_values_list_iterator != tag_values_list.end(); ++tag_values_list_iterator) {
                        mData["Tag Value"] = (*tag_values_list_iterator).toString();
                        tagKeysMenu->addAction((*tag_values_list_iterator).toString(), this, SLOT(dataPlot()))->setData(mData);
                    }
                }
            }
        }
    }
}

bool influxdb_handler::eventFilter(QObject *object, QEvent *event)
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

QJsonDocument influxdb_handler::query(const QUrlQuery &urlQuery)
{
    influxAddress.setQuery(urlQuery);

    // create custom temporary event loop on stack
    QEventLoop loop;
    // "quit()" the event-loop, when the network request "finished()"
    QNetworkAccessManager manager;
    connect(&manager, SIGNAL(finished(QNetworkReply *)), &loop, SLOT(quit()));

    //HTTP request
    QTime timer;
    timer.start();

    QNetworkRequest req(influxAddress);
    QNetworkReply *reply  = manager.get(req);
    //blocks stack until "finished()" has been called
    loop.exec();

    QString strReply = QString(reply->readAll());

    if (reply->error() == QNetworkReply::NoError) {
        queryStats["Query Success"] = queryStats.value("Query Success").toInt() + 1;
        queryStats["Total Latency"] = queryStats.value("Total Latency").toInt() + timer.elapsed();
        queryStats["Received Bytes"] = queryStats.value("Received Bytes").toLongLong() + reply->size();
    } else
        queryStats["Query Fail"] = queryStats.value("Query Fail").toInt() + 1;

    if (strReply.isEmpty())
        return QJsonDocument();

    return QJsonDocument::fromJson(strReply.toUtf8());
}
