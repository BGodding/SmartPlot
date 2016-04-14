#ifndef QCOMPRESSOR_H
#define QCOMPRESSOR_H

#if __APPLE__
#include </usr/include/zlib.h>
#else
#include <QtZlib/zlib.h>
#endif

#include <QByteArray>

//Quazip is also an option, but still relies on zlib

#define GZIP_WINDOWS_BIT MAX_WBITS //use 15 + 16 for gz
#define GZIP_CHUNK_SIZE 32 * 1024

class QCompressor
{
public:
    static bool gzipCompress(QByteArray input, QByteArray &output, int level = -1);
    static bool gzipDecompress(QByteArray input, QByteArray &output);
};

#endif // QCOMPRESSOR_H
