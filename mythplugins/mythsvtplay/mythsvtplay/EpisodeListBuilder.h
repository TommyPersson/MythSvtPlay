#ifndef EPISODELISTBUILDER_H
#define EPISODELISTBUILDER_H

#include <QObject>

#include <QUrl>
#include <QList>
#include <QDate>
#include <QNetworkAccessManager>

class QNetworkReply;
class QDomDocument;

class Episode;

class EpisodeListBuilder : public QObject
{
    Q_OBJECT

public:
    EpisodeListBuilder();

    void buildEpisodeListFromUrl(const QUrl& showUrl);

public slots:
    void onDownloadFinished(QNetworkReply*);

signals:
    void episodesLoaded(const QList<Episode*>& episodes);
    void noEpisodesFound();

private:

    enum DOWNLOAD_STATE { GET_RSS_URL, GET_EPISODES_URLS, GET_EPISODE_DOCS };
    DOWNLOAD_STATE state_;

    void doDownloadFsm();

    QUrl findRssFeed(const QDomDocument& dom);
    QList<QUrl> findEpisodeUrls(const QDomDocument& dom);
    QList<Episode*> parseEpisodeDocs(const QList<QDomDocument>& dom);

    QList<QNetworkReply*> pendingReplies_;
    QList<QNetworkReply*> readyReplies_;

    QNetworkAccessManager manager_;
};

#endif // EPISODELISTBUILDER_H
