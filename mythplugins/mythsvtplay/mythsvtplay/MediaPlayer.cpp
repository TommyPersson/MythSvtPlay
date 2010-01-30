#include "MediaPlayer.h"

#include <QApplication>

#include <mythtv/mythcontext.h>
#include <mythtv/libmythui/mythsystem.h>
#include <mythmainwindow.h>

MediaPlayerWorker::MediaPlayerWorker()
{
    QObject::connect(&cacheCheckTimer_, SIGNAL(timeout()),
                     this, SLOT(onCacheLookUpNeeded()));
    QObject::connect(this, SIGNAL(cacheFilled()),
                     this, SLOT(onCacheFilled()));
}

void MediaPlayerWorker::playEpisode(Episode* episode)
{
    if (episode == NULL)
        return;

    dumper_.setCacheSize(1600000);
    dumper_.dump(episode->mediaUrl, episode->urlIsPlaylist);

    cacheCheckTimer_.start(100);
}

void MediaPlayerWorker::onCacheLookUpNeeded()
{
    int percent = (int) (dumper_.cacheFillRatio() * 100);

    if (percent >= 100)
    {
        cacheCheckTimer_.stop();
        emit cacheFilled();
    }
    else
    {
        emit cacheFilledPercent(percent);
    }
}

void MediaPlayerWorker::onCacheFilled()
{
    gContext->sendPlaybackStart();

    myth_system("mplayer -fs -zoom -ao alsa -really-quiet " + StreamDumper::getDumpFilepath());

    // Internal player is broken on partially downloaded wmv:s, transcoding is needed.
    //gContext->GetMainWindow()->HandleMedia("Internal", StreamDumper::getDumpFilepath(), "plot", "title");

    gContext->sendPlaybackEnd();

    dumper_.abort();
    dumper_.wait();

    emit playbackFinished();
}

MediaPlayer::MediaPlayer()
    : episode_(NULL)
{}

void MediaPlayer::run()
{
    MediaPlayerWorker worker;

    QObject::connect(&worker, SIGNAL(cacheFilledPercent(int)),
                     this, SLOT(onCacheFilledPercentChange(int)));
    QObject::connect(&worker, SIGNAL(cacheFilled()),
                     this, SLOT(onCacheFilled()));
    QObject::connect(&worker, SIGNAL(playbackFinished()),
                     this, SLOT(onPlaybackFinished()));

    worker.playEpisode(episode_);

    exec();
}

void MediaPlayer::playEpisode(Episode* episode)
{
    episode_ = episode;
    start();
}

void MediaPlayer::onCacheFilledPercentChange(int percent)
{
    emit cacheFilledPercent(percent);
}

void MediaPlayer::onCacheFilled()
{
    emit cacheFilled();
}

void MediaPlayer::onPlaybackFinished()
{
    quit();
}
