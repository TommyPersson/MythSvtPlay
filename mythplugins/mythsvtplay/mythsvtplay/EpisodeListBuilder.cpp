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

#include <QBuffer>
#include <QXmlQuery>
#include <QXmlSerializer>

#include <mythtv/mythdirs.h>

#include "Program.h"
#include "Episode.h"

#include <iostream>

static QList<QUrl> findEpisodeUrls(const QDomDocument& dom);
static QList<QDateTime> findEpisodePubDates(const QDomDocument& dom);

static QString findType(const QDomDocument& dom);
static QString findTitle(const QDomDocument& dom);
static QString findDescription(const QDomDocument& dom);
static QString findAvailableUntilDate(const QDomDocument& dom);
static QUrl findMediaUrl(const QDomDocument& dom);
static QUrl findEpisodeImageUrl(const QDomDocument& dom);

static QString executeXQuery(const QDomDocument& dom, const QString& query);

EpisodeListBuilder::EpisodeListBuilder()
        : state_(GET_EPISODES_URLS),
          aborted_(false)
{
    QObject::connect(&manager_, SIGNAL(finished(QNetworkReply*)),
                     this, SLOT(onDownloadFinished(QNetworkReply*)));
    QObject::connect(&imageDownloader_, SIGNAL(finished(QNetworkReply*)),
                     this, SLOT(onDownloadImageFinished(QNetworkReply*)));
}

EpisodeListBuilder::~EpisodeListBuilder()
{
    abort();
}

void EpisodeListBuilder::buildEpisodeList(Program* program)
{
    program_ = program;

    pendingReplies_.clear();
    readyReplies_.clear();
    state_ = GET_EPISODES_URLS;

    QNetworkReply* reply = manager_.get(QNetworkRequest(program->rssUrl));
    pendingReplies_.push_back(reply);
}

void EpisodeListBuilder::abort()
{
    aborted_ = true;

    while (!pendingReplies_.isEmpty())
    {
        QNetworkReply* r = pendingReplies_.takeFirst();
        r->abort();
        r->deleteLater();
    }

    while (!readyReplies_.isEmpty())
    {
        QNetworkReply* r = readyReplies_.takeFirst();
        r->abort();
        r->deleteLater();
    }
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

    QFile savefile(savelocation.absolutePath() + "/" + filename);

    savefile.open(QIODevice::WriteOnly);
    savefile.write(reply->readAll());
    savefile.close();

    reply->deleteLater();
}

void EpisodeListBuilder::doDownloadFsm()
{
    switch(state_)
    {
    case GET_EPISODES_URLS:
        {
            QNetworkReply* reply = readyReplies_.takeFirst();

            QDomDocument rssDoc;
            rssDoc.setContent(reply->readAll());

            QList<QUrl> urls = findEpisodeUrls(rssDoc);

            if (urls.isEmpty())
            {
                pendingReplies_.clear();
                reply->deleteLater();

                emit this->noEpisodesFound();

                return;
            }

            QList<QDateTime> pubDates = findEpisodePubDates(rssDoc);

            for (int i = 0; i < urls.count(); ++i)
            {
                episodeUrlToPubDateMap_.insert(urls.at(i), pubDates.at(i));
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
                QMultiMap<QDateTime, QDomDocument> docs;

                while (!readyReplies_.isEmpty())
                {
                    QNetworkReply* reply = readyReplies_.takeFirst();

                    QDomDocument doc;
                    doc.setContent(reply->readAll());

                    docs.insert(episodeUrlToPubDateMap_.find(reply->url()).value(), doc);

                    reply->deleteLater();
                }

                Program* program = parseEpisodeDocs(docs);

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

    QList<QDateTime> dateList;

    for (int i = 1; i < elements.count(); ++i)
    {
        QString dateString(elements.at(i).toElement().text());

        // Ex: Thu, 07 Jan 2010 18:30:00 GMT
        QDateTime date = QDateTime::fromString(dateString, "ddd, dd MMM yyyy hh:mm:ss 'GMT'");

        dateList.push_back(date);
    }

    return dateList;
}

Program* EpisodeListBuilder::parseEpisodeDocs(const QMultiMap<QDateTime, QDomDocument>& doms)
{
    QList<Episode*> episodeList;

    QMapIterator<QDateTime, QDomDocument> i(doms);
    while (i.hasNext())
    {
        i.next();

        QDomDocument dom = i.value();

        Episode* episode = new Episode;

        episode->type = findType(dom);

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

        if (episode->mediaUrl.path().contains("asx") ||
            episode->mediaUrl.toString().contains("geoip"))
        {
            episode->urlIsPlaylist = true;
        }
        else
        {
            episode->urlIsPlaylist = false;
        }

        episodeList.push_front(episode);
    }

    program_->episodes = episodeList;

    for (int i = 0; i < episodeList.size(); ++i)
    {
        Episode* ep = episodeList.at(i);
        program_->episodesByType.insert(ep->type, ep);
    }

    return program_;
}


QString findTitle(const QDomDocument& dom)
{
    QString wholeTitle = executeXQuery(dom, "string(doc($inputDocument)//title)");

    // A title element is always(?) "{episode title} - {Show} | SVT Play"
    QString title = wholeTitle.left(wholeTitle.lastIndexOf(" - ")+1).simplified();

    return title;
}

QString findDescription(const QDomDocument& dom)
{
    QString description = executeXQuery(dom, "string(doc($inputDocument)//meta[@name='description']/@content)");

    return description;
}

QString findAvailableUntilDate(const QDomDocument& dom)
{
    QString date = executeXQuery(dom, "string(doc($inputDocument)//span[@class='rights']/em)");

    return date;
}

QUrl findMediaUrl(const QDomDocument& dom)
{
    QString mediaUrl = executeXQuery(dom, "string(doc($inputDocument)//a[@class='external-player' and (contains(@href, '.asx') or contains(@href, '.wmv') or contains(@href, '.flv'))][1]/@href)");

    return QUrl(mediaUrl);
}

QUrl findEpisodeImageUrl(const QDomDocument& dom)
{
    QString imageUrl = executeXQuery(dom, "string(doc($inputDocument)//link[@rel='image_src']/@href)");
    imageUrl.replace("thumb","start");

    return QUrl(imageUrl);
}


QString findType(const QDomDocument& dom)
{
    QString type = executeXQuery(dom, "doc($inputDocument)//div[@id='sb']//ul[@class='navigation playerbrowser']//li[@class='selected']//a/text()");

    return type;
}

QString executeXQuery(const QDomDocument& dom, const QString& query)
{
    QBuffer device;
    device.setData(dom.toByteArray());
    device.open(QIODevice::ReadOnly);

    QXmlQuery xquery;
    xquery.bindVariable("inputDocument", &device);
    xquery.setQuery(
            "declare default element namespace \"http://www.w3.org/1999/xhtml\";"
            "declare variable $inputDocument external;" +
            query);

    QString resultString;
    xquery.evaluateTo(&resultString);

    return resultString.trimmed();
}
