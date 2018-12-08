#ifndef PLOT_INTERFACE
#define PLOT_INTERFACE

#include "tabs/plot/plot_handler.h"
#include "tabs/plot/axis_handler.h"


class plot_interface : public QObject
{
    Q_OBJECT

public:
    //plot_interface();

    void selectionChanged(QCustomPlot *customPlot);

    void addToContextMenu(QMenu *menu, QCustomPlot *plot);

public slots:
    void selected_modifyData();
    void selected_remove();
    void selected_rescaleGraph();
    void selected_pickNewLineColor();
    void selected_pickNewFillColor();
    void selected_changeAxis();
    void selectedPlot_stats();
    void selectedPlot_convertUnits();
    void selectedPlot_convert();
    void removeAll();
    void toggleAxisType();

private:
    axis_handler ah;
    plot_handler ph;
};

#endif // PLOT_INTERFACE
