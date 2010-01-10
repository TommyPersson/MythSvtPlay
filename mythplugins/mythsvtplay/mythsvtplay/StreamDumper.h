#ifndef STREAMDUMPER_H
#define STREAMDUMPER_H

#include <QThread>
#include <QUrl>

#include <mythtv/mythdirs.h>

class StreamDumper : public QThread
{
public:
    StreamDumper();
    ~StreamDumper() {}

    void dump(const QUrl& url, bool isPlaylist = false);
    double cacheFillRatio() const;
    void abort();

    static QString getDumpFilepath()
    {
        return GetConfDir() + "/mythsvtplay/tempstream/stream.wmv";
    }

    void run();

private:
    QUrl url_;
    bool isPlaylist_;

};

#endif // STREAMDUMPER_H
