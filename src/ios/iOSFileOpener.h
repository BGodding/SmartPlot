#ifndef IOSFILEOPENER_H
#define IOSFILEOPENER_H

#include <QObject>
#include <QUrl>
#include <QDesktopServices>

class myUrlHandler : public QObject
{   Q_OBJECT

public:
    myUrlHandler(QObject *parent = 0) : QObject(parent){}
    ~ myUrlHandler(){}

public slots:
    void files(const QUrl & url)
    {
      emit openFile(url.toLocalFile());
    }

signals:
    void openFile(const QString&);
};

#endif // FILEOPENER_H
