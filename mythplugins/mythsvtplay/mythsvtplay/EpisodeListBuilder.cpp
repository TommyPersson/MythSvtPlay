#include "EpisodeListBuilder.h"

#include <QUrl>
#include <QNetworkRequest>
#include <QNetworkReply>

#include <QDomDocument>
#include <QDomNode>
#include <QDomNodeList>
#include <QDomNamedNodeMap>

#include <QDir>
#include <QFile>

#include <mythtv/mythdirs.h>

#include "Program.h"
#include "Episode.h"

#include <iostream>

QUrl findRssFeed(const QDomDocument& dom);
QList<QUrl> findEpisodeUrls(const QDomDocument& dom);
QList<QDateTime> findEpisodePubDates(const QDomDocument& dom);

QString findTitle(const QDomDocument& dom);
QString findDescription(const QDomDocument& dom);
QString findAvailableUntilDate(const QDomDocument& dom);
QUrl findMediaUrl(const QDomDocument& dom);
QUrl findEpisodeImageUrl(const QDomDocument& dom);

QString findProgramCategory(const QDomDocument& dom);
QUrl findProgramLogoUrl(const QDomDocument& dom);

EpisodeListBuilder::EpisodeListBuilder()
        : state_(GET_RSS_URL)
{
    QObject::connect(&manager_, SIGNAL(finished(QNetworkReply*)),
                     this, SLOT(onDownloadFinished(QNetworkReply*)));
    QObject::connect(&imageDownloader_, SIGNAL(finished(QNetworkReply*)),
                     this, SLOT(onDownloadImageFinished(QNetworkReply*)));
}

void EpisodeListBuilder::setProgramTitle(const QString& programTitle)
{
    programTitle_ = programTitle;
}

void EpisodeListBuilder::buildEpisodeListFromUrl(const QUrl& showUrl)
{
    pendingReplies_.clear();
    readyReplies_.clear();
    state_ = GET_RSS_URL;

    QNetworkReply* reply = manager_.get(QNetworkRequest(showUrl));
    pendingReplies_.push_back(reply);
}

void EpisodeListBuilder::onDownloadFinished(QNetworkReply* reply)
{
    readyReplies_.push_back(reply);
    pendingReplies_.removeOne(reply);

    doDownloadFsm();
}

void EpisodeListBuilder::onDownloadImageFinished(QNetworkReply* reply)
{
    imageDownloadQueue_.removeOne(reply);

    QUrl url = reply->url();
    QString filename = url.path().section('/', -1);

    QDir savelocation(GetConfDir() + "/mythsvtplay/images/");
    if (!savelocation.exists())
        savelocation.mkpath(savelocation.path());

    QFile savefile(savelocation.absolutePath() + "/" + filename);

    savefile.open(QIODevice::WriteOnly);
    savefile.write(reply->readAll());
    savefile.close();
}

void EpisodeListBuilder::doDownloadFsm()
{
    switch(state_)
    {
    case GET_RSS_URL:
        {
            QNetworkReply* reply = readyReplies_.takeFirst();

            QDomDocument doc;
            doc.setContent(reply->readAll());

            // This will find the program logo on the main program page
            programLogoUrl_ = findEpisodeImageUrl(doc);

            // Likewise
            programDescription_ = findDescription(doc);

            QUrl rssUrl = findRssFeed(doc);

            if (!rssUrl.isValid())
            {
                std::cerr << "The RSS-link was invalid :(" << std::endl;
                reply->deleteLater();
                return;
            }

            QNetworkReply* rssReply = manager_.get(QNetworkRequest(rssUrl));
            pendingReplies_.push_back(rssReply);

            reply->deleteLater();
            state_ = GET_EPISODES_URLS;
            break;
        }
    case GET_EPISODES_URLS:
        {
            QNetworkReply* reply = readyReplies_.takeFirst();

            QDomDocument rssDoc;
            rssDoc.setContent(reply->readAll());

            QList<QUrl> urls = findEpisodeUrls(rssDoc);
            QList<QDateTime> pubDates = findEpisodePubDates(rssDoc);

            for (int i = 0; i < urls.count(); ++i)
            {
                episodeUrlToPubDateMap_.insert(urls.at(i), pubDates.at(i));
            }

            if (urls.isEmpty())
            {
                state_ = GET_RSS_URL;
                pendingReplies_.clear();
                reply->deleteLater();
                
                emit this->noEpisodesFound();
                
                return;
            }

            for (int i = 0; i < urls.size(); ++i)
            {
                QUrl url = urls.at(i);

                QNetworkReply* episodeReply = manager_.get(QNetworkRequest(url));
                pendingReplies_.push_back(episodeReply);
            }

            reply->deleteLater();
            state_ = GET_EPISODE_DOCS;
            break;
        }
    case GET_EPISODE_DOCS:
        {
            if (pendingReplies_.size() == 0)
            {
                QMap<QDateTime, QDomDocument> docsMap;
                //QList<QDomDocument> docs;

                while (!readyReplies_.isEmpty())
                {
                    QNetworkReply* reply = readyReplies_.takeFirst();

                    QDomDocument doc;
                    doc.setContent(reply->readAll());

                    docsMap.insert(episodeUrlToPubDateMap_.find(reply->url()).value(), doc);
                    //docs.push_back(doc);

                    reply->deleteLater();
                }

                Program* program = parseEpisodeDocs(docsMap);

                emit episodesLoaded(program);
            }
            break;
        }
    }
}

void EpisodeListBuilder::downloadImage(const QUrl& url)
{
    QString filename = url.path().section('/', -1);
    QFile file(GetConfDir() + "/mythsvtplay/images/" + filename);

    if (file.exists())
        return;

    QNetworkReply* reply = imageDownloader_.get(QNetworkRequest(url));
    imageDownloadQueue_.push_back(reply);
}

QUrl findRssFeed(const QDomDocument& dom)
{
    QDomNodeList elements = dom.elementsByTagName("link");

    for (int i = 0; i < elements.count(); ++i)
    {
        QDomNamedNodeMap attributes = elements.at(i).attributes();
        if (attributes.contains("rel"))
        {
            if (attributes.namedItem("rel").toAttr().value() == "alternate")
            {
                QUrl url(attributes.namedItem("href").toAttr().value());
                if (url.queryItemValue("expression").contains("full"))
                {
                    return url;
                }
            }
        }
    }
    return QUrl("");
}

QList<QUrl> findEpisodeUrls(const QDomDocument& dom)
{

    QDomNodeList elements = dom.elementsByTagName("link");

    QList<QUrl> urlList;

    for (int i = 0; i < elements.count(); ++i)
    {
        QUrl url(elements.at(i).toElement().text());

        // Disregard the link to the rss-feeds
        if (!url.queryItemValue("mode").contains("rss"))
        {
            urlList.push_back(url);
        }
    }

    return urlList;
}

QList<QDateTime> findEpisodePubDates(const QDomDocument& dom)
{

    QDomNodeList elements = dom.elementsByTagName("pubDate");

    std::cerr << "So far?" << std::endl;

    QList<QDateTime> dateList;

    bool firstIteration = true;
    for (int i = 0; i < elements.count(); ++i)
    {
        // disregard the first date, it's for the entire feed
        if (firstIteration)
        {
            firstIteration = false;
            continue;
        }

        QString dateString(elements.at(i).toElement().text());

        std::cerr << dateString.toStdString() << std::endl;

        // Ex: Thu, 07 Jan 2010 18:30:00 GMT
        QDateTime date = QDateTime::fromString(dateString, "ddd, dd MMM yyyy hh:mm:ss 'GMT'");

        std::cerr << date.toString().toStdString() << std::endl;

        dateList.push_back(date);
    }

    std::cerr << "So far?" << std::endl;
    return dateList;
}

Program* EpisodeListBuilder::parseEpisodeDocs(const QMap<QDateTime, QDomDocument>& doms)
{
    QList<Episode*> episodeList;


    QMapIterator<QDateTime, QDomDocument> i(doms);
    while (i.hasNext())
    {
        i.next();

        QDomDocument dom = i.value();

        Episode* episode = new Episode;

        episode->title = findTitle(dom);
        episode->description = findDescription(dom);

        if (episode->description.simplified() == "")
        {
            episode->description = "Ingen beskrivning tillgänglig.";
        }

        episode->publishedDate = i.key();
        episode->availableUntilDate = QString::fromUtf8("Tillgänglig t.o.m. ") + findAvailableUntilDate(dom) + ".";

        episode->mediaUrl = findMediaUrl(dom);

        QUrl episodeImageUrl = findEpisodeImageUrl(dom);
        downloadImage(episodeImageUrl);
        episode->episodeImageFilepath = GetConfDir() + "/mythsvtplay/images/" + episodeImageUrl.path().section('/', -1);

        if (episode->mediaUrl.path().contains("asx"))
        {
            episode->urlIsPlaylist = true;
        }
        else
        {
            episode->urlIsPlaylist = false;
        }

        episodeList.push_front(episode);
    }

    downloadImage(programLogoUrl_);

    Program* program = new Program();

    program->episodes = episodeList;
    program->logoFilepath = GetConfDir() + "/mythsvtplay/images/" + programLogoUrl_.path().section('/', -1);
    program->title = programTitle_;
    program->description = programDescription_;

    return program;
}

QString findTitle(const QDomDocument& dom)
{
    QDomNodeList nodes = dom.elementsByTagName("title");

    QDomNode node = nodes.at(0); // Only one <title> in XHTML

    QString wholeTitle = node.toElement().text();

    // A title element is always(?) "{episode title} - {Show} | SVT Play"
    QString title = wholeTitle.left(wholeTitle.lastIndexOf(" - ")+1).simplified();

    return title;
}

QString findDescription(const QDomDocument& dom)
{
    QDomNodeList nodes = dom.elementsByTagName("meta");

    for (int i = 0; i < nodes.count(); ++i)
    {
        QDomNamedNodeMap nodeAttributes = nodes.at(i).attributes();

        if (nodeAttributes.contains("name"))
        {
            if (nodeAttributes.namedItem("name").toAttr().value() == "description")
            {
                QString description = nodeAttributes.namedItem("content").toAttr().value();
                return description;
            }
        }
    }

    return "";
}

QString findAvailableUntilDate(const QDomDocument& dom)
{
    QDomNodeList nodes = dom.elementsByTagName("span");

    for (int i = 0; i < nodes.count(); ++i)
    {
        QDomElement node = nodes.at(i).toElement();
        QDomNamedNodeMap nodeAttributes = node.attributes();

        if (nodeAttributes.contains("class"))
        {
            QString classValue = nodeAttributes.namedItem("class").toAttr().value();

            if (classValue == "rights")
            {

                QDomNodeList rightsNodes = node.elementsByTagName("em");

                QString date = rightsNodes.at(0).toElement().text();
                return date;
            }
        }
    }

    return "";
}

QUrl findMediaUrl(const QDomDocument& dom)
{
    QDomNodeList nodes = dom.elementsByTagName("a");

    for (int i = 0; i < nodes.count(); ++i)
    {
        QDomNamedNodeMap nodeAttributes = nodes.at(i).attributes();

        if (nodeAttributes.contains("class"))
        {
            QString classValue = nodeAttributes.namedItem("class").toAttr().value();

            if (classValue == "external-player")
            {
                QString href = nodeAttributes.namedItem("href").toAttr().value();
                QUrl url(href);

                if (url.path().contains("asx") || url.path().contains("wmv"))
                {
                    return url;
                }
            }
        }
    }

    return QUrl("");
}

QUrl findEpisodeImageUrl(const QDomDocument& dom)
{
    QDomNodeList nodes = dom.elementsByTagName("link");

    for (int i = 0; i < nodes.count(); ++i)
    {
        QDomNamedNodeMap nodeAttributes = nodes.at(i).attributes();

        if (nodeAttributes.contains("rel"))
        {
            if (nodeAttributes.namedItem("rel").toAttr().value() == "image_src")
            {
                QString imageUrl = nodeAttributes.namedItem("href").toAttr().value();
                imageUrl.replace("thumb","start");
                return QUrl(imageUrl);
            }
        }
    }

    return QUrl("");
}

QString findProgramCategory(const QDomDocument& dom)
{
    return "";
}
