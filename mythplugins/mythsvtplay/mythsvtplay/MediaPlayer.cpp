#include "MediaPlayer.h"

#include <mythtv/mythcontext.h>
#include <mythtv/libmythui/mythsystem.h>

MediaPlayer::MediaPlayer()
    : episode_(NULL)
{}

void MediaPlayer::run()
{
    play();
}

void MediaPlayer::playEpisode(Episode* episode)
{
    episode_ = episode;
    start();
}

void MediaPlayer::play()
{
    if (episode_ == NULL)
        return;

    QString url = episode_->mediaUrl.toString();

    gContext->sendPlaybackStart();
    if (episode_->urlIsPlaylist)
    {
        std::cerr << "Running: mplayer -fs -zoom -ao alsa -cache 8192 -playlist " << url.toStdString() << std::endl;
        myth_system("mplayer -fs -zoom -ao alsa -cache 8192 -playlist " + url);
    }
    else
    {
        std::cerr << "Running: mplayer -fs -zoom -ao alsa -cache 8192 " << url.toStdString() << std::endl;
        myth_system("mplayer -fs -zoom -ao alsa -cache 8192 " + url);
    }
    //myth_system("vlc " + url);
    gContext->sendPlaybackEnd();
}
