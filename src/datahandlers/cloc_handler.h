#ifndef CLOC_HANDLER
#define CLOC_HANDLER

#include <QMenu>
#include <QSettings>
#include <QSql>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>

#include "text_helper.h"
#include "tabs/plot/plot_handler.h"
#include "tabs/plot/axis_handler.h"

class cloc_handler : public QObject
{
    Q_OBJECT

public:
    cloc_handler();

    void addToSystemMenu(QMenu *menu);
    QPushButton *addToMessageBox(QMessageBox &msgBox);
    void addToContextMenu(QMenu *menu, QCustomPlot* plot);
    void dataImport(QVariantMap modifier);

public slots:
    void updateAxis(QCustomPlot *plot);

protected:
    bool eventFilter(QObject* object,QEvent* event);

private slots:
    void menuDataImport();
    void dataPlot();
    void dataExport(QVariantMap modifier);

private:
    void openDelimitedFile(QString fileName);
    void processLineFromFile(QString line, QString delimiter, QList<QVariantMap> &metaData);
    void connectToClocDatabase(QSqlDatabase *clocData);

//  Series data vectors are stored so the sub vector contains all data in a column
//  This works well as we often want to take and plot a column of data
//  Header: timestamp     | Data Set 1    | Data Set 2    | ... | Data Set x
//  Row 1 : Vector [0][0] | Vector [1][0] | Vector [2][0] | ... | Vector [x][0]
//  Row 2 : Vector [0][1] | Vector [1][1] | Vector [2][1] | ... | Vector [x][1]
//               ...      |      ...      |      ...      | ... |      ...
//  Row y : Vector [0][y] | Vector [1][y] | Vector [2][y] | ... | Vector [x][y]
    QVector<QVector<double> > seriesData;

//  Series:
//  Collection of data that share a measurement, tag set, and retention policy.
//  Example: GCA ADM Data Download, Linesite GDL Download
//  Measurement:
//  Describes the data stored in the associated fields. Measurements are strings.
//  Example: GCA ADM "Event Log", GCA ADM "Diagnostic Log", Linesite "Event", Linesite "Melter Current"
//  Key Field:
//  Field keys are strings and they store metadata
//  Example: "Event Code" Column header in GCA ADM Event log, InvisiPac "value" in LineSite influxDB Melter Current
//  Key Value:
//  Field values are the actual data; they can be strings, floats, integers, or booleans. A field value is always associated with a timestamp.

//  metaData contains all meta information about the columns of data being imported
//  Keys for reference:
//   text "Series"
//   text "Measurement"
//   text "Key Field"  was "Key Field" is the text from the header...pretty obvious
//   text "Key Field Display Text" is the translated string, will be blank if translations not generated
//   variable "Source Location" was "Data Source" is the 0 indexed position of the raw data
//   variable "Storage Location" was "Data Storage" is where in the event or series data vectors the data is stored
//   text "Value Data Type" is the data type ('Series' for time series data, 'Event' for string based data)
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

    int dataDeadTime;

    QSqlDatabase clocData;
};

#endif // cloc_handler
