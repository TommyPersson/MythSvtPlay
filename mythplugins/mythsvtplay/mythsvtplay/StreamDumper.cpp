#include "StreamDumper.h"

#include <QFile>

#include <iostream>

#include <mythtv/libmythui/mythsystem.h>

StreamDumper::StreamDumper()
{
    moveToThread(this);
}

double StreamDumper::cacheFillRatio() const
{
    QFile stream(getDumpFilepath());

    return stream.size() / 1600000.0;
}

void StreamDumper::dump(const QUrl& url, bool isPlaylist)
{
    url_ = url;
    isPlaylist_ = isPlaylist;

    std::cerr << "StreamDumper::dump" << std::endl;

    start();
}

void StreamDumper::abort()
{
    myth_system("ps a | grep mplayer | grep -- \"-dumpfile " + getDumpFilepath() + "\" | cut -f1 -d\" \" | xargs kill -9");

    QFile stream(getDumpFilepath());
    stream.remove();

    terminate();
}

void StreamDumper::run()
{
    std::cerr << "StreamDumper::run" << std::endl;

    if (isPlaylist_)
    {
        myth_system("mplayer -dumpstream -dumpfile " + getDumpFilepath() + " -cache 8192 -playlist " + url_.toString());
    }
    else
    {
        myth_system("mplayer -dumpstream -dumpfile " + getDumpFilepath() + " -cache 8192 " + url_.toString());
    }

    std::cerr << "stream dump \"finished\"" << std::endl;
}
