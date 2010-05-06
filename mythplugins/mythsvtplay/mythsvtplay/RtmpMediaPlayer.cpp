#include "RtmpMediaPlayer.h"

#include <QFile>

#include <mythtv/mythcontext.h>
#include <mythtv/mythdirs.h>

#include <iostream>

#include "Episode.h"

bool isBinaryInPath(QString binary)
{
    QStringList environment = QProcess::systemEnvironment();

    QStringList paths;

    QString str;
    foreach (str, environment)
    {
        if (str.startsWith("PATH="))
        {
            paths = str.mid(5).split(":");
        }
    }

    QString path;
    foreach (path, paths)
    {
        if (QFile(path + "/" + binary).exists())
        {
            return true;
        }
    }

    return false;
}

bool matchesDumperStatusLine(QString string)
{
    // 3713.586 kB / 33.10 sec (2.0%)
    return string.contains("kB /") &&
           string.contains("sec") &&
           string.contains("%");
}

bool matchesDumperClosing(QString string)
{
    return string.contains("Download complete");
}

int getSeconds(QString statusLine)
{
    QString leftClipped = statusLine.mid(statusLine.indexOf("/") + 2);
    QString seconds = leftClipped.mid(0, leftClipped.indexOf("."));

    return seconds.toInt();
}

RtmpMediaPlayer::RtmpMediaPlayer()
    : episode_(NULL),
      dumperState_(STARTING),
      dumperCacheLength_(0),
      quitting_(false)
{
    dumperProcess_.moveToThread(this);

    QObject::connect(&playerProcess_, SIGNAL(finished(int)),
                     this, SLOT(onPlayerFinished(int)));

    QObject::connect(&dumperProcess_, SIGNAL(readyRead()),
                     this, SLOT(onDumperDataAvailable()));
    QObject::connect(&dumperProcess_, SIGNAL(finished(int)),
                     this, SLOT(onDumperFinished(int)));

    QObject::connect(this, SIGNAL(finished()),
                     this, SLOT(deleteLater()));
}

void RtmpMediaPlayer::loadEpisode(Episode* episode)
{
    episode_ = episode;
    start();
}

void RtmpMediaPlayer::run()
{
    QString dumper;
    QString dumperLink;

    if ((!episode_->mediaUrls["rtmps"].toString().isEmpty() ||
         !episode_->mediaUrls["rtmp"].toString().isEmpty()) &&
        isBinaryInPath("rtmpdump"))
    {
        dumper = "rtmpdump";

        dumperLink = !episode_->mediaUrls["rtmps"].toString().isEmpty()
                     ? episode_->mediaUrls["rtmps"].toString()
                     : episode_->mediaUrls["rtmp"].toString();
    }
    else if (!episode_->mediaUrls["rtmp"].toString().isEmpty() &&
             isBinaryInPath("flvstreamer"))
    {
        dumper = "flvstreamer";
        dumperLink = episode_->mediaUrls["rtmp"].toString();
    }

    QStringList dumperArgs;
    dumperArgs << "-r" << dumperLink
               << "-o" << QString(GetConfDir() + "/mythsvtplay/stream.dump");

    dumperProcess_.setReadChannel(QProcess::StandardError);
    dumperProcess_.start(dumper, dumperArgs);

    std::cerr << "Filling cache ..." << std::endl;

    exec();

    quitting_ = true;

    dumperProcess_.close();
    dumperProcess_.kill();

    stopPlaying();

    QFile file(GetConfDir() + "/mythsvtplay/stream.dump");
    file.remove();

    emit playbackFinished();
}

bool RtmpMediaPlayer::canPlay(Episode* episode)
{
    if (!episode->mediaUrls["rtmp"].toString().isEmpty())
    {
        return (isBinaryInPath("flvstreamer") ||
                isBinaryInPath("rtmpdump"));
    }

    if (!episode->mediaUrls["rtmps"].toString().isEmpty())
    {
        return isBinaryInPath("rtmpdump");
    }

    return false;
}

void RtmpMediaPlayer::startPlaying()
{
    QStringList playerArgs;
    playerArgs << "-slave"
               << "-quiet"
               << "-fs"
               << "-zoom"
               << "-ao" << "alsa"
               << GetConfDir() + "/mythsvtplay/stream.dump";

    gContext->sendPlaybackStart();

    std::cerr << "Playing: <" << GetConfDir().toStdString() << "/mythsvtplay/stream.dump" << ">" << std::endl;

    playerProcess_.start("mplayer", playerArgs);

}

void RtmpMediaPlayer::stopPlaying()
{
    playerProcess_.close();
    playerProcess_.kill();

    gContext->sendPlaybackEnd();
}


void RtmpMediaPlayer::onDumperDataAvailable()
{
    QString data(dumperProcess_.readLine(100));

    switch (dumperState_)
    {
    case STARTING:
        {
            if (matchesDumperStatusLine(data))
            {
                dumperState_ = CACHING;
            }
        }
        break;
    case CACHING:
        {
            if (matchesDumperStatusLine(data))
            {
                dumperCacheLength_ = getSeconds(data);

                emit cacheFilledPercent(dumperCacheLength_ * 5);

                if (dumperCacheLength_ * 5 >= 100)
                {
                    startPlaying();
                    dumperState_ = DUMPING;
                    emit cacheFilled();
                }
            }
        }
        break;
    case DUMPING:
        {
            if (matchesDumperStatusLine(data))
            {
                dumperCacheLength_ = getSeconds(data);
            }
            else if (matchesDumperClosing(data))
            {
                dumperState_ = FINISHED;
            }
        }
        break;
    case FINISHED:
        {

        }
        break;
    }
}

void RtmpMediaPlayer::onPlayerFinished(int)
{
    quit();
    emit playbackFinished();
}

void RtmpMediaPlayer::onDumperFinished(int)
{
    if (!quitting_ && dumperState_ == STARTING)
    {
        emit connectionFailed();
        quit();
    }
}
