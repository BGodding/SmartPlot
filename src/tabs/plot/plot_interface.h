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

    void addToContextMenu(QMenu *menu, QCustomPlot* plot);

    enum tickerType { fixed   = 0x01  ///< <tt>0x01</tt> DESCRIPTIONS
                    ,log      = 0x02  ///< <tt>0x02</tt>
                    ,text     = 0x04  ///< <tt>0x04</tt>
                    ,dateTime = 0x08  ///< <tt>0x08</tt>
                    ,time     = 0x10  ///< <tt>0x10</tt>
                  };

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

private:
    axis_handler ah;
    plot_handler ph;
};

#endif // PLOT_INTERFACE
