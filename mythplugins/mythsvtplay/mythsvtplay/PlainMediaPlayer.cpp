#include "PlainMediaPlayer.h"

#include <iostream>

#include <mythtv/mythcontext.h>

bool matchesStatusLine(QString string)
{
    return string.contains("A:") &&
           string.contains("V:") &&
           string.contains("A-V:") &&
           string.contains("ct:");
}

PlainMediaPlayer::PlainMediaPlayer()
    : episode_(NULL),
      mplayerState_(FILLING_CACHE),
      monitorCache_(false),
      quitting_(false)
{
    playerProcess_.moveToThread(this);

    QObject::connect(&playerProcess_, SIGNAL(readyRead()),
                     this, SLOT(onDataAvailable()));

    QObject::connect(&playerProcess_, SIGNAL(finished(int)),
                     this, SLOT(onPlayerFinished(int)));

    QObject::connect(&delayTimer_, SIGNAL(timeout()),
                     this, SLOT(onDelayTimerTimeout()));

    QObject::connect(this, SIGNAL(finished()),
                     this, SLOT(deleteLater()));
}

void PlainMediaPlayer::run()
{    
    QUrl mediaUrl = !episode_->mediaUrls["flv"].toString().isEmpty()
                       ? episode_->mediaUrls["flv"]
                       : episode_->mediaUrls["wmv"];

    QStringList playerArgs;
    playerArgs << "-user-agent" << "NSPlayer/8.0.0.4477"
               << "-slave"
               << "-fs"
               << "-zoom"
               << "-ao" << "alsa"
               << "-cache" << "8192"
               << (episode_->urlIsPlaylist ? "-playlist" : "")
               << mediaUrl.toString();

    std::cerr << "Playing: <" << mediaUrl.toString().toStdString() << ">" << std::endl;

    playerProcess_.start("mplayer", playerArgs);

    std::cerr << "Filling cache ..." << std::endl;

    exec();

    quitting_ = true;

    playerProcess_.close();
    playerProcess_.kill();

    emit playbackFinished();
}

bool PlainMediaPlayer::canPlay(Episode *episode)
{
    return (!episode->mediaUrls["flv"].toString().isEmpty() ||
            !episode->mediaUrls["wmv"].toString().isEmpty());
}

void PlainMediaPlayer::loadEpisode(Episode* episode)
{
    episode_ = episode;
    start();
}

void PlainMediaPlayer::onDataAvailable()
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
                delayTimer_.start(5000);
                mplayerState_ = PLAYING;
                emit cacheFilled();
            }
        }
        break;
    case PLAYING:
        {
            // A:  20.4 V:  20.4 A-V: -0.016 ct: -0.049 386/386  4%  0%  0.4% 0 0 0%
            if (monitorCache_ &&
                matchesStatusLine(data))
            {
                QStringList strings = data.split(" ", QString::SkipEmptyParts);

                QString cacheSizeString = strings.at(strings.length()-2);
                cacheSizeString.replace("%", "");
                int cacheSize = cacheSizeString.toInt();

                if (cacheSize == 0)
                {
                   playerProcess_.write("get_percent_pos\n");
                   playerProcess_.waitForBytesWritten();
                }
            }
            else if (data.contains("ANS_PERCENT_POSITION="))
            {
                int percentPos = data.mid(QString("ANS_PERCENT_POSITION=").length()).toInt();

                if (percentPos < 99)
                {
                    std::cerr << "Caching, again ..." << std::endl;
                    mplayerState_ = CACHING;
                }
            }

        }
        break;
    case CACHING:
        {
            QString osdString;
            osdString = osdString + "osd_show_text \"Buffering for 10 seconds\" 9000\n";

            playerProcess_.write(osdString.toAscii());
            playerProcess_.write(osdString.toAscii());
            playerProcess_.write("pause\n");

            for (int i = 10; i > 0; --i)
            {
                std::cerr << "Buffering, remaning time: " << i << " seconds" << std::endl;
                sleep(1);
            }

            playerProcess_.write("pause\n");

            playerProcess_.read(playerProcess_.bytesAvailable());

            monitorCache_ = false;
            delayTimer_.start(2000);
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

void PlainMediaPlayer::onPlayerFinished(int exitCode)
{
    if (!quitting_ && mplayerState_ == FILLING_CACHE)
    {
        emit connectionFailed();
        quit();
    }
    else if (!quitting_)
    {
        quit();
    }
}

void PlainMediaPlayer::onDelayTimerTimeout()
{
    monitorCache_ = true;
    playerProcess_.read(playerProcess_.bytesAvailable());
    delayTimer_.stop();
}
