#ifndef GIT_CLOC_HANDLER
#define GIT_CLOC_HANDLER

#include <QMenu>
#include <QSettings>
#include <QSql>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>

#include "text_helper.h"
#include "tabs/plot/plot_handler.h"
#include "tabs/plot/axis_handler.h"

class git_cloc_handler : public QObject
{
    Q_OBJECT

public:
    git_cloc_handler();

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
    void openSqlFile(const QString &fileName);
    void processLineFromFile(QString line, QString delimiter, QList<QVariantMap> &metaData);
    void connectToClocDatabase(QSqlDatabase *clocData);

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

    QSqlDatabase clocData;
};

#endif // git_cloc_handler
