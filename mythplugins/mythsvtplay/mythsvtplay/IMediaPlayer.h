#ifndef IMEDIAPLAYER_H
#define IMEDIAPLAYER_H

#include <QThread>

class Episode;

class IMediaPlayer : public QThread
{
    Q_OBJECT

public:
    virtual ~IMediaPlayer() {}

    virtual void loadEpisode(Episode* episode) = 0;

    virtual void run() = 0;

public slots:
    virtual void onDataAvailable() = 0;
    virtual void onPlayerFinished(int) = 0;
    virtual void onDelayTimerTimeout() = 0;

signals:
    void cacheFilledPercent(int percent);
    void cacheFilled();
    void playbackFinished();

};

#endif // IMEDIAPLAYER_H
