#ifndef INFLUXDB_HANDLER_H
#define INFLUXDB_HANDLER_H

#include <QMenu>
#include <QJsonDocument>
#include <QtNetwork>

#include "tabs/plot/plot_handler.h"
#include "tabs/plot/axis_handler.h"
#include "tabs/plot/qcustomplot.h"
#include "json_helper.h"

//TODO: Add query wrapper to batch requests
class influxdb_handler : public QObject
{
    Q_OBJECT
public:
    influxdb_handler();

    void addToSystemMenu(QMenu *menu, QCustomPlot *plot);
    void addToContextMenu(QMenu *menu, QCustomPlot *plot);
    void close();
//    void dataImport(QVariantMap modifier);

public slots:
//    void updateAxis(QCustomPlot *plot);

private slots:
    void menuRefresh();
    void menuConfigure();
    void dataPlot();
//    void dataExport(QVariantMap modifier);

protected:
    bool eventFilter(QObject *object, QEvent *event);

private:
    void generateMetaData();

    void setInfluxAddress(QUrl url);
    QJsonDocument query(const QUrlQuery &urlQuery);

    QVector<QVector<double> > seriesData;
    QVector<QVector<QString> > eventData;
    QList<QVariantMap> metaData;

    int maxVerticalEvents{};
    QUrl influxAddress;

    QMenu contextMenu;

    QCustomPlot *influxPlot{};

    axis_handler ah;
    plot_handler ph;
    json_helper jh;

    QVariantMap queryStats;
};

#endif // INFLUXDB_HANDLER_H
