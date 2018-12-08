#ifndef SMART_PLOT
#define SMART_PLOT

#include <QMainWindow>

//Data Handlers: Add header files here
#include "datahandlers/csv_handler.h"
#include "datahandlers/influxdb_handler.h"
#include "datahandlers/git_cloc_handler.h"

#include "ios/iOSFileOpener.h"

#include "tabs/plot/qcustomplot.h"
#include "tabs/plot/axis_handler.h"
#include "tabs/plot/plot_interface.h"

/* Line Chart
 * Time series data
 * Time
 * Value
 *
 * Axis Label
 * Event
 * Time
 * n possible additional tags, for example
 * -Event(code, description, action, type)
 * -Software(part number, serial, version)
 *
 * Bar Chart
 * BlackBox (Time, Description, Value)
 *
 * Bar Chart
 * Aggrigate(date made from time series data)
 * Time
 *
 * Bar Chart
 * Material logs
 * Time (period)
 * n possible additional values
 *
 */

class smart_plot : public QMainWindow
{
    Q_OBJECT

public:
    explicit smart_plot(QWidget *parent = nullptr);

protected:
    bool eventFilter(QObject *object, QEvent *ev);
    void resizeEvent(QResizeEvent *event);
    void closeEvent(QCloseEvent *);

private slots:
    void rangeChanged(const QCPRange &newRange);
    void mousePress(QMouseEvent *event);
    void selectionChanged();
    void contextMenuRequest(QPoint pos);

    //Called by qcustomplot slot to rename the title
    void titleDoubleClick(QMouseEvent *event);
    //Called by qcustomplot slot to rename the selected axis
    void axisDoubleClick(QCPAxis *axis, QCPAxis::SelectablePart part);
    //Called by qcustomplot slot to rename plot in legend
    void legendDoubleClick(QCPLegend *legend, QCPAbstractLegendItem *item);

    void iosOpen(const QString &fileName);

    void zoomInButtonPressed();
    void zoomOutButtonPressed();

    void savePlotAsImage();

private:
    void initLegend(QCustomPlot *customPlot);
    void initGraph(QCustomPlot *customPlot);
    QCustomPlot *activePlot();

    QVector<QCustomPlot *> plots;
    plot_interface plotInterface;
    axis_handler axisHandler;
    csv_handler csvHandler;
    influxdb_handler influxdbHandler;
    git_cloc_handler gitClocHandler;

    QMenu *contextMenu;

    QString url;
};

#endif // SMART_PLOT
