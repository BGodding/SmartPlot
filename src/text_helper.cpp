#include "text_helper.h"

#include <QMessageBox>
#include <QPushButton>
#include <QDebug>
#include <utility>

QString text_helper::autoDetectDelimiter(QTextStream &stream)
{
    const QString delimiters[] = {"\t", ",", " "};
    qint64 streamStartingPosition = stream.pos();
    QVector<QString> possibleDelimiters;
    QString line = QString();
    QStringList strings;
    int i;

    while (!stream.atEnd() && (possibleDelimiters.size() != 1)) {
        line = QString(stream.readLine());
        possibleDelimiters.clear();

        if (line.isEmpty())
            continue;

        for (i = 0; i < 3; i++) {
            strings.clear();
            strings = line.split(delimiters[i]);
            if ( strings.size() != 1 ) {
                possibleDelimiters.append(delimiters[i]);
            }
        }
    }
    //qDebug() << "Best guess from auto delim format->" << possibleDelimiters.first() << possibleDelimiters.size();

    //Reposition the stream
    stream.seek(streamStartingPosition);

    if (!possibleDelimiters.empty()) {
        return possibleDelimiters.first();
    }

    //Something went wrong, lets pick whatever is first and run with it. Maybe there is only 1 column
    return delimiters[0];
}

bool text_helper::firstColumnIsIncrimental(QTextStream &stream, const QString &delimiter)
{
    qint64 streamStartingPosition = stream.pos();
    double firstColumnValue, prevFirstColumnValue = 0;
    bool firstColumnIsIncrimental = true;
    QString line = QString();
    QStringList strings;
    bool ok;

    //Read out the first line (possibly header)
    line = QString(stream.readLine());

    while (!stream.atEnd()) {
        line = QString(stream.readLine());
        QStringList strings = line.split(delimiter);

        //Lets determine if we can use the first column as an X-axis
        firstColumnValue = strings.value(0).toDouble(&ok);

        if (!ok || (firstColumnValue < prevFirstColumnValue)) {
            firstColumnIsIncrimental = false;
            break;
        }

        prevFirstColumnValue = firstColumnValue;
    }

    //Reposition the stream
    stream.seek(streamStartingPosition);

    return firstColumnIsIncrimental;
}

QString text_helper::cleanDateTimeString(const QString &rawDate, const QString &rawTime)
{
    //Really dumb, but the best thing I can think of :/
    QString workString = " ";

    //Generate combined string
    workString.append(rawDate);
    workString.append(" ");
    workString.append(rawTime);

    //Format string
    workString.replace(QRegExp(" 0"), " " );
    workString.replace(QRegExp("/0"), "/" );
    workString.replace(QRegExp(":0"), ":" );

    return workString;
}

void text_helper::checkAndProcessColumnHeaders( QTextStream &stream, const QString &delimiter,
                                                QList<QVariantMap> &metaData, int firstDataColumn  )
{
    qint64 streamStartingPosition = stream.pos();

    QString line = QString(stream.readLine());
    QStringList strings = line.split(delimiter);
    bool headerFound = true;
    int column;

    QVariantMap variantMap;

    //Check if the first line is not a header (only contains spaces, numbers, decimal points)
    QRegExp re("^[ .0-9]*$");
    if (re.exactMatch(line)) {
        //reset stream and return
        stream.seek(streamStartingPosition);
        headerFound = false;
    }

    for (column = firstDataColumn; column < strings.size(); column++) {
        variantMap.clear();
        if (headerFound) {
            if (strings.value(column).size() != 0) {
                variantMap["Key Field"] = strings.value(column);
                variantMap["Data Source"] = column;
            }
        } else {
            //Use column number as the header
            variantMap["Key Field"] = QString(tr("Column ")) + QString::number(column);
            variantMap["Data Source"] = column;
        }

        if (!variantMap.isEmpty())
            metaData.append(variantMap);
    }
}

int text_helper::versionStringToInt(QString version)
{
    bool ok;
    //Versions are stored in x.yy.zzz format
    return version.remove('.').toInt(&ok);
}

QString text_helper::versionIntToString(int version)
{
    //Versions are stored in x.yy.zzz format
    QString versionString = QString::number(version);
    versionString = versionString.insert(1, ".");
    return versionString.insert(4, ".");
}

bool text_helper::isVersionOk(QString minVersion, QString maxVersion, const QString &readVersion)
{
    if ( (versionStringToInt(readVersion) <= versionStringToInt(maxVersion))
            && (versionStringToInt(readVersion) >= versionStringToInt(minVersion)) )
        return true;
    else
        return false;
}

int text_helper::estimateLineCount( const QString &fileName )
{
    QFile file(fileName);
    qint64 fileStartingPosition = 0;
    int lineCount = 0;

    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream textStream(&file);
        textStream.setAutoDetectUnicode(true);

        while (!textStream.atEnd() && (++lineCount < 150) ) {
            if (lineCount == 50)
                fileStartingPosition = textStream.pos();

            textStream.readLine();
        }
        qint64 byteCount = abs(textStream.pos() - fileStartingPosition);

        file.close();

        if (lineCount < 150)
            return lineCount;

        return int((file.size() * 100) / byteCount);
    }
    return 0;
}

void text_helper::insertDataBreak(double dateTime, QVector<QVector<double> > &seriesData,
                                  QVector<QVector<QString> > &eventData, QList<QVariantMap> &metaData)
{
    //Add the date and time
    seriesData.first().append(dateTime);

    //Iterate through meta data and append data as needed
    for (auto &mData : metaData) {
        if (mData.contains("Data Type")) {
            if (mData.value("Data Type") == "Series") {
                seriesData[mData.value("Data Value Storage Index").toInt()].append(
                    std::numeric_limits<double>::quiet_NaN());
            }
        }
    }
    if ( !eventData.isEmpty() ) {
        QVector<QString> spacerVector(eventData.first().size());
        eventData.append(spacerVector);
    }
}
