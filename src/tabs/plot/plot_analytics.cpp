#include "plot_analytics.h"

#include "utility.h"
#include "tabs/plot/qcustomplot.h"


plotStats::plotStats(double average) :
    totalData_seconds(0),
    totalNonZeroData_seconds(0),
    totalData_entrys(0),
    totalValue_DeltaP2P(0),
    maxValue(0),
    maxPosValue_DeltaP2P(0),
    maxNegValue_DeltaP2P(0),
    minValue(std::numeric_limits<double>::max()),
    minValue_DeltaP2P(std::numeric_limits<double>::max()),
    avgValue(average),
    longestTimeNoDelta_seconds(0),
    longestTimeDelta_seconds(0),
    totalTimeNoDelta_seconds(0),
    totalTimeDelta_seconds(0),
    currentTimeNoDelta_seconds(0),
    currentTimeDelta_seconds(0)
{
}

plotStats::plotStats() :
    totalData_seconds(0),
    totalNonZeroData_seconds(0),
    totalData_entrys(0),
    totalValue_DeltaP2P(0),
    maxValue(0),
    maxPosValue_DeltaP2P(0),
    maxNegValue_DeltaP2P(0),
    minValue(std::numeric_limits<double>::max()),
    minValue_DeltaP2P(std::numeric_limits<double>::max()),
    avgValue(0),
    longestTimeNoDelta_seconds(0),
    longestTimeDelta_seconds(0),
    totalTimeNoDelta_seconds(0),
    totalTimeDelta_seconds(0),
    currentTimeNoDelta_seconds(0),
    currentTimeDelta_seconds(0)
{
}

void plot_analytics::plotAnalyze( QCPGraph *target, plotStats *stats)
{
    plotAnalyze(target, stats, target->keyAxis()->range());
}

void plot_analytics::plotAnalyze( QCPGraph *target, QVector<plotStats> *stats, double iterator)
{
    plotAnalyze( target, stats, target->keyAxis()->range(), iterator);
}

void plot_analytics::plotAnalyze( QCPGraph *target, plotStats *stats, QCPRange keyRange)
{
    QCPGraphDataContainer::const_iterator plotIterator = target->data().data()->findBegin(keyRange.lower, true);
    QCPGraphDataContainer::const_iterator targetDataEnd = target->data().data()->findEnd(keyRange.upper, true);
    QCPGraphData currentPoint, prevPoint;

    //Find the data ranges
    currentPoint.key = keyRange.lower;
    prevPoint = QCPGraphData(plotIterator->key, plotIterator->value);

    while (plotIterator != targetDataEnd && (keyRange.contains(plotIterator->key) || stats->totalData_entrys < 2)) {
        currentPoint = QCPGraphData(plotIterator->key, plotIterator->value);
        handlePoints(currentPoint, prevPoint, stats);

        //Value is weighted to account for datapoints that may not be equally spaced
        if (!isInvalidData(currentPoint.value) && !isInvalidData(prevPoint.value))
            stats->avgValue += currentPoint.value * (currentPoint.key - prevPoint.key);

        prevPoint = QCPGraphData(plotIterator->key, plotIterator->value);
        ++plotIterator;
    }
    //Divide by total seconds
    stats->avgValue /= stats->totalData_seconds;
}

void plot_analytics::plotAnalyze( QCPGraph *target, QVector<plotStats> *stats, QCPRange keyRange, double iterator)
{
    plotStats currentRangeStats;
    QCPRange currentRange = keyRange;

    currentRange.upper = currentRange.lower + iterator;

    while (keyRange.contains(currentRange.upper)) {
        //Gernate and append the stats
        plotAnalyze( target, &currentRangeStats, currentRange);
        stats->append(currentRangeStats);
        //Update working range
        currentRange.lower = currentRange.upper;
        currentRange.upper += iterator;
    }
}

void plot_analytics::plotAnalyze( QCPGraph *target, plotStats *stats, QCPRange keyRange, QCPGraph *enableReference, QCPRange valueEnableRange)
{
    clearUnwantedPoints(target, enableReference, valueEnableRange);
    plotAnalyze(target, stats, keyRange);
}

void plot_analytics::plotAnalyze( QCPGraph *target, QVector<plotStats> *stats, QCPRange keyRange, QCPGraph *enableReference, QCPRange valueEnableRange, double iterator)
{
    //Iterate here
    plotStats currentRangeStats;
    QCPRange currentRange = keyRange;

    currentRange.upper = currentRange.lower + iterator;
    while (keyRange.contains(currentRange.upper)) {
        //Gernate and append the stats
        plotAnalyze( target, &currentRangeStats, keyRange, enableReference, valueEnableRange);
        stats->append(currentRangeStats);
        //Update working range
        currentRange.lower = currentRange.upper;
        currentRange.upper += iterator;
        currentRangeStats = plotStats();
    }
}

bool plot_analytics::handlePoints(double targetValue, double referenceValue, double keyDelta, plotStats *stats)
{
    QCPGraphData target(keyDelta, targetValue);
    QCPGraphData reference(0, referenceValue);

    return handlePoints(target, reference, stats);
}

bool plot_analytics::handlePoints(QCPGraphData target, QCPGraphData reference, plotStats *stats)
{
    if ( (!isInvalidData(target.key, target.value)) && (!isInvalidData(reference.key, reference.value)) ) {
        //Determine how much time was betwen points
        double dataKeyDeltaP2P = qAbs(target.key - reference.key);

        //Determine how much the value changed
        double dataValueDeltaP2P = target.value - reference.value;

        //qDebug() << dataKeyDeltaP2P << dataValueDeltaP2P << stats->totalValue_Sum;

        stats->totalValue_DeltaP2P += qAbs(dataValueDeltaP2P) * dataKeyDeltaP2P;
        stats->totalData_seconds += dataKeyDeltaP2P;
        stats->totalData_entrys++;

        if (dataValueDeltaP2P > stats->maxPosValue_DeltaP2P)
            stats->maxPosValue_DeltaP2P = dataValueDeltaP2P;

        if (dataValueDeltaP2P < stats->maxNegValue_DeltaP2P)
            stats->maxNegValue_DeltaP2P = dataValueDeltaP2P;

        if (qAbs(dataValueDeltaP2P) < stats->minValue_DeltaP2P)
            stats->minValue_DeltaP2P = qAbs(dataValueDeltaP2P);

        if (target.value > stats->maxValue)
            stats->maxValue = target.value;

        if (target.value < stats->minValue)
            stats->minValue = target.value;

        if (target.key != 0)
            stats->totalNonZeroData_seconds += dataKeyDeltaP2P;

        if (dataValueDeltaP2P == 0) {
            stats->totalTimeNoDelta_seconds += dataKeyDeltaP2P;
            stats->currentTimeDelta_seconds += dataKeyDeltaP2P;
            stats->currentTimeNoDelta_seconds = 0;

            if (stats->currentTimeDelta_seconds > stats->longestTimeDelta_seconds)
                stats->longestTimeDelta_seconds = stats->currentTimeDelta_seconds;
        } else {
            stats->totalTimeDelta_seconds += dataKeyDeltaP2P;
            stats->currentTimeDelta_seconds = 0;
            stats->currentTimeNoDelta_seconds += dataKeyDeltaP2P;

            if (stats->currentTimeNoDelta_seconds > stats->longestTimeNoDelta_seconds)
                stats->longestTimeNoDelta_seconds = stats->currentTimeNoDelta_seconds;
        }

        return true;
    }
    return false;
}

void plot_analytics::clearUnwantedPoints(QCPGraph *target, QCPGraph *enableReference, QCPRange valueEnableRange)
{
    QCPGraphDataContainer::iterator plotIterator = target->data()->begin();
    QCPGraphDataContainer::const_iterator refIterator;

    //First we scan and any time the reference plot is not in range wipe the plot data.
    //TODO: This will break the plot, do we clone the plot? That could be a huge amount of data...
    while (plotIterator != target->data()->end()) {
        //Find closest reference point
        refIterator = enableReference->data().data()->findBegin(plotIterator->key, true);
        if (!valueEnableRange.contains(refIterator->value))
            plotIterator->value = std::numeric_limits<double>::quiet_NaN();

        ++plotIterator;
    }
}

void plot_analytics::clearUnwantedPoints(QCPGraph *target, QCPGraph *newTarget, QCPGraph *enableReference, QCPRange valueEnableRange)
{
    //Copy one graph to another or delete this function?
    clearUnwantedPoints(newTarget, enableReference, valueEnableRange);
}
