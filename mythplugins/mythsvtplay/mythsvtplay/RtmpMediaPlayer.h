#ifndef RTMPMEDIAPLAYER_H
#define RTMPMEDIAPLAYER_H

#include "IMediaPlayer.h"

#include <QProcess>
#include <QTimer>

class RtmpMediaPlayer : public IMediaPlayer
{
    Q_OBJECT

public:
    RtmpMediaPlayer();
    ~RtmpMediaPlayer() {}

    void loadEpisode(Episode* episode);

    void run();

    static bool canPlay(Episode* episode);

public slots:
    void onPlayerDataAvailable();
    void onDumperDataAvailable();

    void onPlayerFinished(int);
    void onDumperFinished(int);
    void onCheckCacheTimeout();

private:

    void startPlaying();
    void stopPlaying();

    enum DumperState { STARTING, CACHING, DUMPING, FINISHED };

    Episode* episode_;

    QProcess playerProcess_;
    QProcess dumperProcess_;

    DumperState dumperState_;

    int dumperCacheLength_;

    QTimer checkCacheTimer_;

    bool quitting_;
};

#endif // RTMPMEDIAPLAYER_H
