#ifndef PLOT_HANDLER
#define PLOT_HANDLER

#include <QObject>

#include "tabs/plot/qcustomplot.h"
#include "tabs/plot/axis_handler.h"

class plot_handler : public QObject
{
    Q_OBJECT

public:
    plot_handler();

    QCPGraph *addPlotLine(QVector<QVector<double> > &dataVector, QVariantMap metaData);
    QCPGraph *addPlotLine(QVector<double> &key, QVector<double> &value, const QString &name, QCustomPlot *plot);
    QCPGraph *addPlotLine(QCPGraphDataContainer *dataMap, const QString &name, QCustomPlot *plot);

    void plotConvert( QCPGraph *graph, const QString &functionString );

    void plotAddPeriodicReport(QCPGraph *graph, QVariantMap metaData);
private slots:
private:
    void generatePenColor(QPen *pen);

    QList < QColor> penColors;

    axis_handler ah;
};

#endif // PLOT_HANDLER

