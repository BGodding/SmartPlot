# Smartplot
A flexible and fast data visualization tool built with Qt

## Features
* Fast! Runs on [QCustomPlot](https://www.qcustomplot.com/) and supports hardware acceleration
* Currently supports delimited files plus a few custom types. Should be easy to support new types.
* Modify plots with [Qt's QJSEngine](http://doc.qt.io/qt-5/qjsengine.html)

![Example Plot](https://i.imgur.com/RO7Kcfv.png)

## Getting Started
1. Run the Exe or build from source
2. File -> OpenDataFile and select delimited file type. If the data has a headers lines that will be used as column names
3. Right click to display imported data, left click to plot.

TLDR: Right click always brings up the context menu based on what is selected.  
CTRL + left click to select multiple items
Double click to change text.
Middle click to view value for the points under the mouse + min, avg, max values for that series in the current window.
Middle click then CTRL + middle click to show Delta values