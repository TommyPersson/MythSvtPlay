#ifndef MEDIAPLAYER_H
#define MEDIAPLAYER_H

#include <QProcess>
#include <QThread>
#include <QTimer>

#include "IMediaPlayer.h"
#include "Episode.h"

class MediaPlayer : public IMediaPlayer
{
    Q_OBJECT

public:
    MediaPlayer();
    ~MediaPlayer() {}

    void loadEpisode(Episode* episode);
    
    void run();

    static bool canPlay(Episode* episode);

public slots:
    void onDataAvailable();
    void onPlayerFinished(int);
    void onDelayTimerTimeout();

private:

    enum MplayerState { FILLING_CACHE, PLAYING, CACHING, RESUMING };

    Episode* episode_;
    QProcess playerProcess_;

    MplayerState mplayerState_;

    QTimer delayTimer_;

    bool monitorCache_;
    bool quitting_;
};

#endif // MEDIAPLAYER_H
