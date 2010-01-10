#ifndef EPISODELISTBUILDER_H
#define EPISODELISTBUILDER_H

#include <QObject>

#include <QMap>
#include <QUrl>
#include <QList>
#include <QDateTime>
#include <QNetworkAccessManager>

class QNetworkReply;
class QDomDocument;

class Program;

class EpisodeListBuilder : public QObject
{
    Q_OBJECT

public:
    EpisodeListBuilder();

    void setProgramTitle(const QString& programTitle);

    void buildEpisodeListFromUrl(const QUrl& showUrl);

public slots:
    void onDownloadFinished(QNetworkReply*);
    void onDownloadImageFinished(QNetworkReply*);

signals:
    void episodesLoaded(Program* program);
    void noEpisodesFound();

private:

    enum DOWNLOAD_STATE { GET_RSS_URL, GET_EPISODES_URLS, GET_EPISODE_DOCS };
    DOWNLOAD_STATE state_;

    void doDownloadFsm();

    void downloadImage(const QUrl& url);

    Program* parseEpisodeDocs(const QMap<QDateTime, QDomDocument>& dom);

    QString programTitle_;
    QString programDescription_;
    QUrl programLogoUrl_;

    QMap<QUrl, QDateTime> episodeUrlToPubDateMap_;

    QList<QNetworkReply*> pendingReplies_;
    QList<QNetworkReply*> readyReplies_;

    QList<QNetworkReply*> imageDownloadQueue_;

    QNetworkAccessManager manager_;
    QNetworkAccessManager imageDownloader_;
};

#endif // EPISODELISTBUILDER_H
