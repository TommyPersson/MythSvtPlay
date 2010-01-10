#include "MediaPlayer.h"

#include <QApplication>

#include <mythtv/mythcontext.h>
#include <mythtv/libmythui/mythsystem.h>
#include <mythmainwindow.h>


MediaPlayerWorker::MediaPlayerWorker(QObject* parent)
{
    QObject::connect(&cacheCheckTimer_, SIGNAL(timeout()),
                     this, SLOT(onCacheLookUpNeeded()));
    QObject::connect(this, SIGNAL(cacheFilled()),
                     this, SLOT(onCacheFilled()));
}

void MediaPlayerWorker::playEpisode(Episode* episode)
{
    //std::cerr << "MediaPlayerWorker::playEpisode" << std::endl;

    if (episode == NULL)
        return;

    dumper_.dump(episode->mediaUrl, episode->urlIsPlaylist);

    cacheCheckTimer_.start(200);

    std::cerr << "Timer started?" << std::endl;
}

void MediaPlayerWorker::onCacheLookUpNeeded()
{
    //std::cerr << "MediaPlayerWorker::onCacheLookUpNeeded" << std::endl;

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
    //std::cerr << "MediaPlayerWorker::onCacheFilled" << std::endl;

    gContext->sendPlaybackStart();

    //myth_system("mplayer -fs -zoom -ao alsa " + StreamDumper::getDumpFilepath());

    gContext->GetMainWindow()->HandleMedia("Internal", StreamDumper::getDumpFilepath());

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
    std::cerr << "MediaPlayer::run()" << std::endl;

    MediaPlayerWorker worker(this);

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
    //std::cerr << "MediaPlayer::onCacheFilledPercentChange" << std::endl;
    emit cacheFilledPercent(percent);

    QCoreApplication::processEvents();
}

void MediaPlayer::onCacheFilled()
{
    //std::cerr << "MediaPlayer::onCacheFilled" << std::endl;
    emit cacheFilled();

    QCoreApplication::processEvents();
}

void MediaPlayer::onPlaybackFinished()
{
    //std::cerr << "MediaPlayer::onPlaybackFinished" << std::endl;
    quit();
}
