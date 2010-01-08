#ifndef MEDIAPLAYER_H
#define MEDIAPLAYER_H

#include <QThread>

#include "Episode.h"

class MediaPlayer : public QThread
{
public:
    MediaPlayer();

    void loadEpisode(Episode* episode);
    
    void run();

private:
    void play();

    Episode* episode_;
};

#endif // MEDIAPLAYER_H
