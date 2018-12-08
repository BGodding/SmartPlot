#include "smart_plot.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    QTranslator qtTranslator;
    qtTranslator.load(":/qttranslations/qt_" + QLocale::system().name());
    QApplication::installTranslator(&qtTranslator);

    QTranslator myappTranslator;
    myappTranslator.load(":/translations/translation_" + QLocale::system().name());
    QApplication::installTranslator(&myappTranslator);

    smart_plot w;
    w.show();

    return QApplication::exec();
}
