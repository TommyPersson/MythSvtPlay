#include "EpisodeListBuilder.h"

#include <QUrl>
#include <QNetworkRequest>
#include <QNetworkReply>

#include <QDomDocument>
#include <QDomNode>
#include <QDomNodeList>
#include <QDomNamedNodeMap>

#include "Episode.h"

#include <iostream>

QString findTitle(const QDomDocument& dom);
QString findDescription(const QDomDocument& dom);
QDate findPublishedDate(const QDomDocument& dom);
QDate findAvailableUntilDate(const QDomDocument& dom);
QUrl findMediaUrl(const QDomDocument& dom);
QUrl findEpisodeImageUrl(const QDomDocument& dom);

EpisodeListBuilder::EpisodeListBuilder()
        : state_(GET_RSS_URL)
{
    QObject::connect(&manager_, SIGNAL(finished(QNetworkReply*)),
                     this, SLOT(onDownloadFinished(QNetworkReply*)));
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

void EpisodeListBuilder::doDownloadFsm()
{
    switch(state_)
    {
    case GET_RSS_URL:
        {
            QNetworkReply* reply = readyReplies_.takeFirst();

            QDomDocument doc;
            doc.setContent(reply->readAll());

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
                QList<QDomDocument> docs;

                while (!readyReplies_.isEmpty())
                {
                    QNetworkReply* reply = readyReplies_.takeFirst();

                    QDomDocument doc;
                    doc.setContent(reply->readAll());
                    docs.push_back(doc);

                    reply->deleteLater();
                }

                QList<Episode*> episodes = parseEpisodeDocs(docs);

                emit episodesLoaded(episodes);
            }
            break;
        }
    }
}

QUrl EpisodeListBuilder::findRssFeed(const QDomDocument& dom)
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

QList<QUrl> EpisodeListBuilder::findEpisodeUrls(const QDomDocument& dom)
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

QList<Episode*> EpisodeListBuilder::parseEpisodeDocs(const QList<QDomDocument>& doms)
{
    QList<Episode*> episodeList;

    for (int i = 0; i < doms.count(); ++i)
    {
        QDomDocument dom = doms.at(i);
        Episode* episode = new Episode;

        std::cerr << "is here!" << std::endl;

        episode->title = findTitle(dom);
        episode->description = findDescription(dom);

        episode->publishedDate = findPublishedDate(dom);
        episode->availableUntilDate = findAvailableUntilDate(dom);

        episode->mediaUrl = findMediaUrl(dom);
        episode->episodeImageUrl = findEpisodeImageUrl(dom);

        if (episode->mediaUrl.path().contains("asx"))
        {
            episode->urlIsPlaylist = true;
        }
        else
        {
            episode->urlIsPlaylist = false;
        }

        episodeList.push_back(episode);
    }

    return episodeList;
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

QDate findPublishedDate(const QDomDocument& dom)
{
    return QDate();
}

QDate findAvailableUntilDate(const QDomDocument& dom)
{
    return QDate();
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
    return QUrl("");
}
