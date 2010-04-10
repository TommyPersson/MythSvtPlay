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

class EpisodeListBuilder : public QObject
{
    Q_OBJECT

public:
    EpisodeListBuilder();
    ~EpisodeListBuilder();

    void buildEpisodeList(Program*);
    void abort();

public slots:
    void onDownloadFinished(QNetworkReply*);
    void onDownloadImageFinished(QNetworkReply*);

signals:
    void episodesLoaded(Program* program);
    void noEpisodesFound();

private:

    enum DOWNLOAD_STATE { GET_EPISODES_URLS, GET_EPISODE_DOCS };
    DOWNLOAD_STATE state_;

    void doDownloadFsm();

    void downloadImage(const QUrl& url);

    Program* parseEpisodeDocs(const QMultiMap<QDateTime, QDomDocument>& dom);

    Program* program_;

    bool aborted_;

    QMap<QUrl, QDateTime> episodeUrlToPubDateMap_;

    QList<QNetworkReply*> pendingReplies_;
    QList<QNetworkReply*> readyReplies_;

    QList<QNetworkReply*> imageDownloadQueue_;

    QNetworkAccessManager manager_;
    QNetworkAccessManager imageDownloader_;
};

#endif // EPISODELISTBUILDER_H
