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

signals:
    void connectionFailed();
    void cacheFilledPercent(int percent);
    void cacheFilled();
    void playbackFinished();

};

#endif // IMEDIAPLAYER_H
