#include "MediaPlayer.h"

#include <iostream>

#include <mythtv/mythcontext.h>

bool matchesStatusLine(QString string)
{
    return string.contains("A:") &&
           string.contains("V:") &&
           string.contains("A-V:") &&
           string.contains("ct:");
}

MediaPlayer::MediaPlayer()
    : episode_(NULL),
      mplayerState_(FILLING_CACHE),
      monitorCache_(false)
{
    playerProcess_.moveToThread(this);

    QObject::connect(&playerProcess_, SIGNAL(readyRead()),
                     this, SLOT(onDataAvailable()));

    QObject::connect(&playerProcess_, SIGNAL(finished(int)),
                     this, SLOT(onPlayerFinished(int)));

    QObject::connect(&delayTimer_, SIGNAL(timeout()),
                     this, SLOT(onDelayTimerTimeout()));
}

void MediaPlayer::run()
{
    QStringList playerArgs;
    playerArgs << "-user-agent" << "NSPlayer/8.0.0.4477"
               << "-slave"
               << "-fs"
               << "-zoom"
               << "-ao" << "alsa"
               << "-cache" << "8192"
               << (episode_->urlIsPlaylist ? "-playlist" : "")
               << episode_->mediaUrl.toString();

    gContext->sendPlaybackStart();

    playerProcess_.start("mplayer", playerArgs);

    std::cerr << "Filling cache ..." << std::endl;

    exec();

    playerProcess_.close();
    monitorCache_ = false;
    mplayerState_ = FILLING_CACHE;
    gContext->sendPlaybackEnd();
}

void MediaPlayer::loadEpisode(Episode* episode)
{
    episode_ = episode;
    start();
}

void MediaPlayer::onDataAvailable()
{
    QString data(playerProcess_.readLine(100));

    switch (mplayerState_)
    {
    case FILLING_CACHE:
        {
            if (data.contains("Cache fill:"))
            {
                //Cache fill:  0.00% (0 bytes)
                int beginPos = data.indexOf(":");
                int endPos = data.indexOf("%");
                QString floatData = data.mid(beginPos + 1, endPos - beginPos - 1);

                float percent = floatData.toFloat();
                int upscaledPercent = (int) percent*5;
                emit cacheFilledPercent(upscaledPercent);
            }
            else if (matchesStatusLine(data))
            {
                std::cerr << "Starting playback ..." << std::endl;
                delayTimer_.start(3000);
                mplayerState_ = PLAYING;
                emit cacheFilled();
            }
        }
        break;
    case PLAYING:
        {
            // A:  20.4 V:  20.4 A-V: -0.016 ct: -0.049 386/386  4%  0%  0.4% 0 0 0%
            if (matchesStatusLine(data) &&
                monitorCache_)
            {
                QStringList strings = data.split(" ", QString::SkipEmptyParts);

                QString cacheSizeString = strings.at(strings.length()-2);
                cacheSizeString.replace("%", "");
                int cacheSize = cacheSizeString.toInt();

                if (cacheSize <= 1)
                {
                   std::cerr << "Caching, again ..." << std::endl;
                   mplayerState_ = CACHING;
                }
            }
        }
        break;
    case CACHING:
        {
            playerProcess_.write("pause\n");

            for (int i = 10; i > 0; --i)
            {
                std::cerr << "Buffering, remaning time: " << i << " seconds" << std::endl;

                QString osdString;
                osdString = osdString + "osd_show_text \"Buffering (" + QString::number(i) + ") \" 1000\n";

                // Mplayer sometimes needs to be kicked multiple times to actually display something
                playerProcess_.write("pause\n");
                playerProcess_.write(osdString.toAscii());
                playerProcess_.write("pause\n");
                playerProcess_.write(osdString.toAscii());
                playerProcess_.write("pause\n");

                sleep(1);
            }

            playerProcess_.write("pause\n");

            playerProcess_.read(playerProcess_.bytesAvailable());

            mplayerState_ = RESUMING;
        }
        break;
    case RESUMING:
        {
            if (matchesStatusLine(data))
            {
                QStringList strings = data.split(" ", QString::SkipEmptyParts);

                QString cacheSizeString = strings.at(strings.length()-2);
                cacheSizeString.replace("%", "");
                int cacheSize = cacheSizeString.toInt();

                QString osdString;
                osdString = osdString + "pausing_keep osd_show_text \"Cache size: " +  QString::number(cacheSize) + "%\" 4000\n";

                // Mplayer sometimes needs to be kicked multiple times to actually display something
                playerProcess_.write(osdString.toAscii());
                playerProcess_.write(osdString.toAscii());

                std::cerr << "Resuming playback ..." << std::endl;
                mplayerState_ = PLAYING;
            }
        }
        break;
    }
}

void MediaPlayer::onPlayerFinished(int exitCode)
{
    quit();
}

void MediaPlayer::onDelayTimerTimeout()
{
    monitorCache_ = true;
    playerProcess_.read(playerProcess_.bytesAvailable());
}
