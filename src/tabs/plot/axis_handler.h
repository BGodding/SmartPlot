#ifndef AXIS_HANDLER
#define AXIS_HANDLER

#include <QObject>
#include "tabs/plot/qcustomplot.h"

class axis_handler : public QObject
{
    Q_OBJECT

public:
    enum tickerType { fixed   = 0x01  ///< <tt>0x01</tt> DESCRIPTIONS
    , log      = 0x02 ///< <tt>0x02</tt>
    , text     = 0x04 ///< <tt>0x04</tt>
    , dateTime = 0x08 ///< <tt>0x08</tt>
    , time     = 0x10 ///< <tt>0x10</tt>
                    };

    void updateGraphAxes(QCustomPlot *plot);
    void updateAxis(QCustomPlot *plot, QList<QVariantMap> &metaData, QVector<QVector<QString> > &eventData, QMap<QString, QMap<int, QString> > &tickLabelLookup, QVector<double> &datetime, QCPAxis *axis,
                    int maxVerticalEvents);
    void updateAxis(QCustomPlot *plot, QList<QVariantMap> &metaData, const QJsonDocument &jsondoc, QCPAxis *axis, int maxVerticalEvents);
    void updateAxisTickCount(QCustomPlot *customPlot, QWidget *window);

    bool isEventVisible(QList<QVariantMap> &metaData, QVector<QVector<QString> > &eventData, int row, QString &tickLabel );
    bool isEventVisible(QList<QVariantMap> &metaData, const QJsonArray &eventData, QString &tickLabel );
    bool isActionVisible(QVariantMap &selectionData, QList<QVariantMap> &metaData);

    void toggleKeyValueVisibleInList(QVariantMap &selectionData, QList<QVariantMap> &metaData);
    void toggleKeyFieldVisibleInList(QVariantMap &selectionData, QList<QVariantMap> &metaData);

    bool isAxisSelected(QCPAxis *axis);
    bool isAxisSelected(QCustomPlot *plot, QCPAxis::AxisType AxisTypes);
    bool isAxisSelected(QCustomPlot *plot, QCPAxis::AxisType AxisTypes, QCPAxis::SelectablePart part);
    bool isAxisTypeSelected(QCustomPlot *customPlot, QCPAxis::AxisType type);

    void setAxisSelected(QCPAxis *axis);
    void setAxesSelected(const QList<QCPAxis *> &axis);
    void setAxesSelected(QCustomPlot *plot, QCPAxis::AxisType type);
    void setAxisType(QCPAxis *axis, tickerType type);
    void toggleAxisType(QCPAxis *axis);
};

#endif // AXIS_HANDLER
