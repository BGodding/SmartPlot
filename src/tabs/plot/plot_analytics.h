#ifndef PLOT_ANALYTICS_H
#define PLOT_ANALYTICS_H

#include <QString>

#include "tabs/plot/qcustomplot.h"

class QCP_LIB_DECL plotStats
{
public:
    plotStats();
    plotStats(double average);
    double totalData_seconds;
    double totalNonZeroData_seconds;
    int totalData_entrys;
    double totalValue_DeltaP2P;
    double maxValue;
    double maxPosValue_DeltaP2P;
    double maxNegValue_DeltaP2P;
    double minValue;
    double minValue_DeltaP2P;
    double avgValue;
    double longestTimeNoDelta_seconds;
    double longestTimeDelta_seconds;
    double totalTimeNoDelta_seconds;
    double totalTimeDelta_seconds;
    double currentTimeNoDelta_seconds;
    double currentTimeDelta_seconds;
};
Q_DECLARE_TYPEINFO(plotStats, Q_MOVABLE_TYPE);

class plot_analytics
{

public:

    //For plotStats Functions
    //P2P = Point to Point, the change between sequential points on the same plot
    //For plotDiffStats Funcitons
    //P2P = Plot to Plot, the difference betweeen two plots at a point in time

    //Generate plot stats over the entire key range of the target plot
    //The iterator enables stats data to be run at fixed intervals so the stats can be plotted over time
    void plotAnalyze( QCPGraph *target, plotStats *stats);
    void plotAnalyze( QCPGraph *target, QVector<plotStats> *stats, double iterator);

    //Generate plot stats over a fixed key range
    void plotAnalyze( QCPGraph *target, plotStats *stats, QCPRange keyRange);
    void plotAnalyze( QCPGraph *target, QVector<plotStats> *stats, QCPRange keyRange, double iterator);

    //Generate plot stats over a fixed key range, only generating data when enableReference value is within the valueEnableRange bounds
    void plotAnalyze( QCPGraph *target, plotStats *stats, QCPRange keyRange, QCPGraph *enableReference, QCPRange valueEnableRange);
    void plotAnalyze( QCPGraph *target, QVector<plotStats> *stats, QCPRange keyRange, QCPGraph *enableReference, QCPRange valueEnableRange, double iterator);

//    void ultraAnalyze(QDir targetDir, QString nameMustContain, QVector<int> targetColumn, QVector<plotStats> *stats);
//    void ultraAnalyze(QDir targetDir, QString nameMustContain, QStringList targetColumnName, QVector<plotStats> *stats);
//    void ultraAnalyze(QDir targetDir, QRegExp nameRegExp, QString targetColumnName);
    //Returns the first point before the target point
private:
    bool handlePoints(double targetValue, double referenceValue, double keyDelta, plotStats *stats);
    bool handlePoints(QCPGraphData target, QCPGraphData reference, plotStats *stats);

    //Target will be modifed
    void clearUnwantedPoints(QCPGraph *target, QCPGraph *enableReference, QCPRange valueEnableRange);
    //Target will be cloned
    void clearUnwantedPoints(QCPGraph *target, QCPGraph *newTarget, QCPGraph *enableReference, QCPRange valueEnableRange);
};

#endif // PLOT_ANALYTICS_H
