#ifndef CSV_HANDLER
#define CSV_HANDLER

#include <QMenu>
#include <QSettings>

#include "text_helper.h"
#include "tabs/plot/plot_handler.h"
#include "tabs/plot/axis_handler.h"

class csv_handler : public QObject
{
    Q_OBJECT

public:
    csv_handler();

    void addToSystemMenu(QMenu *menu, QCustomPlot *plot);
    QPushButton *addToMessageBox(QMessageBox &msgBox, QCustomPlot *plot);
    void addToContextMenu(QMenu *menu, QCustomPlot *plot);
    void dataImport(const QVariantMap &modifier);

public slots:
    void updateAxis(QCustomPlot *plot);

protected:
    bool eventFilter(QObject *object, QEvent *event);

private slots:
    void menuDataImport();
    void dataPlot();
    void dataExport(QVariantMap modifier);

private:
    void openDelimitedFile(const QString &fileName);
    void processLineFromFile(const QString &line, const QString &delimiter, int dataKeyColumn, int metaDataIndexStart, QList<QVariantMap> &metaData);
//  Series data vectors are stored so the sub vector contains all data in a column
//  This works well as we often want to take and plot a column of data
//  Header: timestamp     | Data Set 1    | Data Set 2    | ... | Data Set x
//  Row 1 : Vector [0][0] | Vector [1][0] | Vector [2][0] | ... | Vector [x][0]
//  Row 2 : Vector [0][1] | Vector [1][1] | Vector [2][1] | ... | Vector [x][1]
//               ...      |      ...      |      ...      | ... |      ...
//  Row y : Vector [0][y] | Vector [1][y] | Vector [2][y] | ... | Vector [x][y]
    QVector<QVector<double> > seriesData;

    //TODO: Does this break with the multiple file option?
//  Event data vectors is stored so the primary vector contains all data in a row
//  This works well as we often want to look at all the event data at a point in time
//  Header: timestamp     | Event Type 1  | Event Type 2  | ... | Event Type x
//  Row 1 : Vector [0][0] | Vector [0][1] | Vector [0][2] | ... | Vector [0][x]
//  Row 2 : Vector [1][0] | Vector [1][1] | Vector [1][2] | ... | Vector [1][x]
//               ...      |      ...      |      ...      | ... |      ...
//  Row y : Vector [y][0] | Vector [y][1] | Vector [y][2] | ... | Vector [y][x]
    QVector<QVector<QString> > eventData;

//  [measurement][index position of matching]
    QMap<QString, QMap<int, QString> > tickLabelLookup;

//  Series:
//  Collection of data that share a measurement, tag set, and retention policy.
//  Measurement:
//  Describes the data stored in the associated fields. Measurements are strings.
//  Key Field:
//  Field keys are strings and they store metadata
//  Key Value:
//  Field values are the actual data; they can be strings, floats, integers, or booleans. A field value is always associated with a timestamp.

//  metaData contains all meta information about the columns of data being imported
//  Keys for reference:
//   text "Series"
//   text "Measurement"
//   text "Key Field"  was "Key Field" is the text from the header...pretty obvious
//   text "Key Field Display Text" is the translated string, will be blank if translations not generated
//   variable "Source Location" was "Data Source" is the 0 indexed position of the raw data
//   variable "Storage Location" was "Data Value Storage Index" is where in the event or series data vectors the data is stored
//   text "Data Type" is the data type ('Series' for time series data, 'Event' for string based data)
//   -----Keys below apply to Data Type = 'Event' Only!-----
//   text "Action" how to handle if the entry is selected ('OR', 'AND')
//   bool "Tick Label" is the text is displayed on the x axis when selected
//   -> QList<QVariantMap> "Unique Event Meta Data"
//   --> bool "Active" is the entry has been selected
//   --> text "Key Value"
//   --> text "Key Value Display Text" will be blank if translations not generated
    QList<QVariantMap> metaData;

    text_helper th;
    axis_handler ah;
    plot_handler ph;

    int maxVerticalEvents;
    int maxPlotSize;
    int dataDeadTime;
};

#endif // csv_handler
