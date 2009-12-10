#include "EpisodeListBuilder.h"

#include <QUrl>
#include <QNetworkRequest>
#include <QNetworkReply>

#include <QDomDocument>
#include <QDomNode>
#include <QDomNodeList>
#include <QDomNamedNodeMap>

EpisodeListBuilder::EpisodeListBuilder(const QUrl& showUrl)
        : state_(GET_RSS_URL)
{
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
}

QList<QUrl> EpisodeListBuilder::findEpisodeUrls(const QDomDocument& dom)
{
}

QList<Episode*> EpisodeListBuilder::parseEpisodeDocs(const QList<QDomDocument>& dom)
{
}
