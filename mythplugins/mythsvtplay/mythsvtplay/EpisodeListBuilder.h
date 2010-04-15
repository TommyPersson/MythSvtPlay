#ifndef EPISODELISTBUILDER_H
#define EPISODELISTBUILDER_H

#include <QObject>

#include <QMultiMap>
#include <QUrl>
#include <QList>
#include <QDateTime>
#include <QNetworkAccessManager>

class QNetworkReply;
class QDomDocument;

class Program;
class Episode;

class EpisodeListBuilder : public QObject
{
    Q_OBJECT

public:
    EpisodeListBuilder(QString episodeType, QUrl url);

    ~EpisodeListBuilder();

    void buildEpisodeList();

    bool isBusy();

    bool moreEpisodesAvailable();

    QList<Episode*> episodeList();

    void abort();

public slots:
    void onDownloadFinished(QNetworkReply*);
    void onDownloadImageFinished(QNetworkReply*);

signals:
    void episodesReady(const QString& episodeType);
    void noEpisodesFound(const QString& episodeType);

private:

    enum DOWNLOAD_STATE { GET_EPISODES_URLS, GET_EPISODE_DOCS };
    DOWNLOAD_STATE state_;

    void doDownloadFsm();

    void downloadImage(const QUrl& url);

    Episode* parseEpisodeDoc(const QDomDocument& dom);

    bool aborted_;

    bool episodesAvailable_;

    bool busy_;

    QUrl pageUrl_;
    QString episodeType_;

    QMap<int, Episode*> episodes_;

    QList<QNetworkReply*> pendingReplies_;
    QList<QNetworkReply*> readyReplies_;

    QList<QNetworkReply*> imageDownloadQueue_;

    QNetworkAccessManager manager_;
    QNetworkAccessManager imageDownloader_;
};

#endif // EPISODELISTBUILDER_H
