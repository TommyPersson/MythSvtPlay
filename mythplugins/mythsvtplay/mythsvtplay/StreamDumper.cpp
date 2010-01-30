#include "StreamDumper.h"

#include <QFile>
#include <QProcess>

#include <mythtv/libmythui/mythsystem.h>

StreamDumper::StreamDumper()
{
    dumpProcess_.moveToThread(this);
}

void StreamDumper::setCacheSize(unsigned int cacheSize)
{
    cacheSize_ = cacheSize;
}

double StreamDumper::cacheFillRatio() const
{
    QFile stream(getDumpFilepath());

    return stream.size() / (double) cacheSize_;
}

void StreamDumper::dump(const QUrl& url, bool isPlaylist)
{
    url_ = url;
    isPlaylist_ = isPlaylist;

    start();
}

void StreamDumper::abort()
{
    dumpProcess_.kill();

    QFile stream(getDumpFilepath());
    stream.remove();

    terminate();
}

void StreamDumper::run()
{
    QStringList dumpArgs;
    dumpArgs << "-user-agent" << "NSPlayer/8.0.0.4477"
             << "-really-quiet"
             << "-cache" << "8192"
             << "-dumpstream"
             << "-dumpfile" << getDumpFilepath();

    if (isPlaylist_)
    {
        dumpArgs << "-playlist";
    }

    QFile stream(getDumpFilepath());
    if (stream.exists())
    {
        stream.remove();
    }

    dumpArgs << url_.toString();

    dumpProcess_.start("mplayer", dumpArgs);
    dumpProcess_.waitForFinished();
}
