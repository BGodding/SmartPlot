#ifndef UTILITY
#define UTILITY

#include <QVariantMap>
#include <QVector>
#include <QString>
#include <QList>
#include <QMenu>
#include "tabs/plot/qcustomplot.h"

inline bool isInvalidData(double value)
{
    return qIsNaN(value) || qIsInf(value);
}

inline bool isInvalidData(double value1, double value2)
{
    return isInvalidData(value1) || isInvalidData(value2);
}

void generateUniqueLists( QVector<QVector<QString> > *data, QList<QVariantMap> &uniqueEventMetaData);
void generateUniqueLists( QVector<QVector<QString> > *data, QList<QVariantMap> &metaData, const QMap<int, int> &referencePairs);
void generateUniqueList(const QVector<QString> &data, QList<QString> &list);
void generateUniqueList(QVector<QVector<QString> > *data, int column, QList<QString> &list);
QMap<QString, QString> generateUniqueListWithRef(QVector<QVector<QString> > *data, int pColumn, int rColumn);
QPen setPenAlpha(QPen pen, int alpha);
void generatePenColor(QPen *pen);
void IncrementDateTime( const QVariantMap &metaData, QDateTime *dateTime );
void scaleMenuForScreen(QMenu *menu, QCustomPlot *plot);
QString seconds_to_DHMS(quint32 duration);

#endif // UTILITY
