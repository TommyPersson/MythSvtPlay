#ifndef STREAMDUMPER_H
#define STREAMDUMPER_H

#include <QProcess>
#include <QThread>
#include <QUrl>

#include <mythtv/mythdirs.h>

class StreamDumper : public QThread
{
    Q_OBJECT

public:
    StreamDumper();
    ~StreamDumper() {}

    void dump(const QUrl& url, bool isPlaylist = false);
    double cacheFillRatio() const;
    void abort();

    void setCacheSize(unsigned int cacheSize);

    static QString getDumpFilepath()
    {
        return GetConfDir() + "/mythsvtplay/tempstream/stream.wmv";
    }

    void run();

private:
    QProcess dumpProcess_;
    QProcess recodeProcess_;
    QUrl url_;
    bool isPlaylist_;

    unsigned int cacheSize_;

};

#endif // STREAMDUMPER_H
