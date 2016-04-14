#ifndef AXIS_HANDLER
#define AXIS_HANDLER

#include <QObject>
#include "tabs/plot/qcustomplot.h"

class axis_handler : public QObject
{
    Q_OBJECT

public:
    void updateGraphAxes(QCustomPlot *plot);
    void updateAxis(QCustomPlot *plot, QList<QVariantMap> &metaData, QVector<QVector<QString> > &eventData, QMap<QString, QMap<int, QString> > &tickLabelLookup, QVector<double> &datetime, QCPAxis *axis, int maxVerticalEvents);
    void updateAxis(QCustomPlot *plot, QList<QVariantMap> &metaData, QJsonDocument jsondoc, QCPAxis *axis, int maxVerticalEvents);

    bool isEventVisible(QList<QVariantMap> &metaData, QVector<QVector<QString> > &eventData, int row, QString &tickLabel );
    bool isEventVisible(QList<QVariantMap> &metaData, QJsonArray eventData, QString &tickLabel );
    bool isActionVisible(QVariantMap &selectionData, QList<QVariantMap> &metaData);

    void toggleKeyValueVisibleInList(QVariantMap &selectionData, QList<QVariantMap> &metaData);
    void toggleKeyFieldVisibleInList(QVariantMap &selectionData, QList<QVariantMap> &metaData);

    bool isAxisSelected(QCPAxis* axis);
    bool isAxisSelected(QCustomPlot* plot, QCPAxis::AxisType AxisTypes);
    bool isAxisTypeSelected(QCustomPlot *customPlot, QCPAxis::AxisType type);

    void setAxisSelected(QCPAxis* axis);
    void setAxesSelected(QCustomPlot *plot, QCPAxis::AxisType type);
};

#endif // AXIS_HANDLER
