#include "smart_plot.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    QTranslator qtTranslator;
    qtTranslator.load(":/qttranslations/qt_" + QLocale::system().name());
    a.installTranslator(&qtTranslator);

    QTranslator myappTranslator;
    myappTranslator.load(":/translations/translation_" + QLocale::system().name());
    a.installTranslator(&myappTranslator);

    smart_plot w;
    w.show();

    return a.exec();
}
