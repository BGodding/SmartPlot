#-------------------------------------------------
#
# Project created by QtCreator
#
#-------------------------------------------------

QT       += core gui qml
#Needed for influx
QT       += network

TARGET = smartplot

#uncomment out the lines below to build with mobile/touch support
#DEFINES += MOBILE
QMAKE_INFO_PLIST = Info.plist
OTHER_FILES += Info.plist

TEMPLATE = app

ios {
    ios_icon.files = $$files($$PWD/ios/AppIcon*.png)
    QMAKE_BUNDLE_DATA += ios_icon
}

macx {
    LIBS += -lz
}

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets printsupport

SOURCES += main.cpp \
    smart_plot.cpp \
    #qcompressor.cpp \
    json_helper.cpp \
    text_helper.cpp \
    utility.cpp \
    datahandlers/csv_handler.cpp \
    datahandlers/influxdb_handler.cpp \
    tabs/plot/axis_handler.cpp \
    tabs/plot/plot_analytics.cpp \
    tabs/plot/plot_handler.cpp \
    tabs/plot/plot_interface.cpp \
    tabs/plot/qcustomplot.cpp

HEADERS += \
    smart_plot.h \
    #qcompressor.h \
    json_helper.h \
    text_helper.h \
    utility.h \
    ios/iOSFileOpener.h \
    datahandlers/csv_handler.h \
    datahandlers/influxdb_handler.h \
    tabs/plot/axis_handler.h \
    tabs/plot/plot_analytics.h \
    tabs/plot/plot_handler.h \
    tabs/plot/plot_interface.h \
    tabs/plot/qcustomplot.h

RC_FILE = smartplot.rc

RESOURCES += \
    resources/smartplot.qrc

TRANSLATIONS += \
    resources/qt_de.ts \
    resources/qt_es.ts \
    resources/qt_fr.ts \
    resources/qt_it.ts \
    resources/qt_ja.ts \
    resources/qt_ko.ts \
    resources/qt_pt.ts \
    resources/qt_zh.ts
