#ifndef MEDIAPLAYER_H
#define MEDIAPLAYER_H

#include <QThread>
#include <QTimer>

#include "Episode.h"
#include "StreamDumper.h"

class MediaPlayerWorker : public QObject
{
    Q_OBJECT

public:
    MediaPlayerWorker();
    ~MediaPlayerWorker() {};

    void playEpisode(Episode* episode);

public slots:
    void onCacheFilled();
    void onCacheLookUpNeeded();

signals:
    void cacheFilledPercent(int percent);
    void cacheFilled();
    void playbackFinished();

private:
    void play();

    QTimer cacheCheckTimer_;

    StreamDumper dumper_;
};

class MediaPlayer : public QThread
{
    Q_OBJECT

public:
    MediaPlayer();
    ~MediaPlayer() {}

    void playEpisode(Episode* episode);
    
    void run();

public slots:
    void onCacheFilledPercentChange(int);
    void onCacheFilled();
    void onPlaybackFinished();

signals:
    void cacheFilledPercent(int percent);
    void cacheFilled();

private:
    Episode* episode_;
};

#endif // MEDIAPLAYER_H
