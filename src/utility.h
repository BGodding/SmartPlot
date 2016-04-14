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
void generateUniqueList(const QVector<QString> &data, QList<QString> &list);
void generateUniqueList(QVector<QVector<QString> > *data, int column, QList<QString> &list);
QPen setPenAlpha(QPen pen, int alpha);
void generatePenColor(QPen *pen);
void IncrementDateTime( QVariantMap metaData, QDateTime *dateTime );
void scaleMenuForScreen(QMenu* menu, QCustomPlot* plot);
QString seconds_to_DHMS(quint32 duration);

#endif // UTILITY
