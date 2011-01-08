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

#include <QRegExp>
#include <QtAlgorithms>

#include <mythtv/mythdirs.h>

#include "Program.h"
#include "Episode.h"

#include "Settings.h"

#include <iostream>

static QList<QUrl> findEpisodeUrls(const QDomDocument& dom);
static QList<QDateTime> findEpisodePubDates(const QDomDocument& dom);

static QString findType(const QDomDocument& dom);
static QString findTitle(const QDomDocument& dom);
static QString findDescription(const QDomDocument& dom);
static QString findAvailableUntilDate(const QDomDocument& dom);
static QMap<QString, QUrl> findMediaUrls(const QDomDocument& dom);
static QUrl findEpisodeImageUrl(const QDomDocument& dom);
static QString findNextPageQueryString(const QDomDocument& dom);
static void addDirectories(QMap<int, IProgramItem*>& items, const QDomDocument& dom);

static QString executeXQuery(const QDomDocument& dom, const QString& query);

static QDomDocument* parseSvtPlayReply(QNetworkReply* reply);

EpisodeListBuilder::EpisodeListBuilder(QString episodeType, QUrl url)
    : state_(GET_EPISODES_URLS),
      aborted_(false),
      episodesAvailable_(true),
      pageUrl_(url),
      episodeType_(episodeType),
      busy_(false)
{
    QObject::connect(&episodeDownloader_, SIGNAL(finished(QNetworkReply*)),
                     this, SLOT(onDownloadFinished(QNetworkReply*)),
                     Qt::QueuedConnection);
    QObject::connect(&imageDownloader_, SIGNAL(finished(QNetworkReply*)),
                     this, SLOT(onDownloadImageFinished(QNetworkReply*)),
                     Qt::QueuedConnection);
}

EpisodeListBuilder::~EpisodeListBuilder()
{
    abort();

    for (int i = 0; i < episodes_.values().count(); ++i)
    {
        delete episodes_.values().at(i);
    }
    episodes_.clear();
}

void EpisodeListBuilder::buildEpisodeList()
{
    pendingReplies_.clear();
    readyReplies_.clear();
    state_ = GET_EPISODES_URLS;

    std::cerr << "Downloading episodes for <" << episodeType_.toStdString() << "> at <" << pageUrl_.toString().toStdString() << ">" << std::endl;

    busy_ = true;

    QNetworkReply* reply = episodeDownloader_.get(QNetworkRequest(pageUrl_));
    pendingReplies_.push_back(reply);
}

bool EpisodeListBuilder::moreEpisodesAvailable()
{
    return episodesAvailable_;
}

bool EpisodeListBuilder::isBusy()
{
    return busy_;
}

void EpisodeListBuilder::abort()
{
    aborted_ = true;

    episodeDownloader_.blockSignals(true);
    imageDownloader_.blockSignals(true);

    while (!pendingReplies_.isEmpty())
    {
        QNetworkReply* r = pendingReplies_.takeFirst();
        r->abort();
    }

    while (!readyReplies_.isEmpty())
    {
        QNetworkReply* r = readyReplies_.takeFirst();
        r->abort();
    }

    while (!imageDownloadQueue_.isEmpty())
    {
        QNetworkReply* r = imageDownloadQueue_.takeFirst();
        r->abort();
    }
}

QList<IProgramItem*> EpisodeListBuilder::episodeList()
{
    return episodes_.values();
}

void EpisodeListBuilder::onDownloadFinished(QNetworkReply* reply)
{
    if (aborted_)
    {
        return;
    }

    readyReplies_.push_back(reply);
    pendingReplies_.removeOne(reply);

    doDownloadFsm();
}

void EpisodeListBuilder::onDownloadImageFinished(QNetworkReply* reply)
{
    if (aborted_)
    {
        return;
    }

    imageDownloadQueue_.removeOne(reply);

    QUrl url = reply->url();
    QString filename = url.path().section('/', -1);

    QDir savelocation(GetConfDir() + "/mythsvtplay/images/");

    QFile savefile(savelocation.absolutePath() + "/" + filename);

    savefile.open(QIODevice::WriteOnly);
    savefile.write(reply->readAll());
    savefile.close();
}

void EpisodeListBuilder::doDownloadFsm()
{
    switch(state_)
    {
    case GET_EPISODES_URLS:
        {
            QNetworkReply* reply = readyReplies_.takeFirst();

            QDomDocument* doc = parseSvtPlayReply(reply);

            reply->deleteLater();

            if (doc != NULL)
            {
                addDirectories(episodes_, *doc);

                QList<QUrl> urls = findEpisodeUrls(*doc);

                QString query = findNextPageQueryString(*doc);

                if(!query.isEmpty())
                {
                    episodesAvailable_ = true;
                    pageUrl_.setEncodedQuery(query.toUtf8());
                }
                else
                {
                    episodesAvailable_ = false;
                }

                int offset = episodes_.count();
                if (offset != 0)
                {
                    offset = episodes_.values().last()->position + 1;
                }

                for (int i = 0; i < urls.size(); ++i)
                {
                    QUrl url = urls.at(i);

                    if (!url.toString().contains("playprima"))
                    {
                        QNetworkRequest request(url);
                        request.setAttribute(QNetworkRequest::User, QVariant(offset + i));

                        QNetworkReply* episodeReply = episodeDownloader_.get(request);
                        pendingReplies_.push_back(episodeReply);
                    }
                }
            }
            state_ = GET_EPISODE_DOCS;
            doDownloadFsm();
            break;
        }
    case GET_EPISODE_DOCS:
        {
            if (pendingReplies_.isEmpty())
            {
                while (!readyReplies_.isEmpty())
                {
                    QNetworkReply* reply = readyReplies_.takeFirst();

                    QDomDocument* doc = parseSvtPlayReply(reply);

                    if (doc != NULL)
                    {
                        IProgramItem* episode = parseEpisodeDoc(*doc);

                        if (episode != NULL)
                        {
                            episode->position = reply->request().attribute(QNetworkRequest::User).toInt();
                            episodes_[episode->position] = episode;
                        }
                    }

                    reply->deleteLater();
                }

                busy_ = false;

                emit episodesReady(episodeType_);
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
    QString results = executeXQuery(dom,
                                    "<urls> "
                                    "{for $a in reverse(doc($inputDocument)//div[@id='sb']//div[contains(@class, 'show-tab-container')]//li//a[not(contains(@class, 'folder'))]) "
                                    "return "
                                    "<url>{string($a/@href)}</url>} "
                                    "</urls>\n");

    QDomDocument xqueryResults;
    xqueryResults.setContent(results);

    QList<QUrl> urlList;

    QDomNodeList urls = xqueryResults.elementsByTagName("url");

    for (int i = 0; i < urls.count(); ++i)
    {
        urlList.push_front(QUrl("http://svtplay.se/" + urls.at(i).toElement().text()));
    }

    return urlList;
}

void addDirectories(QMap<int, IProgramItem*>& items, const QDomDocument& dom)
{
    QString results = executeXQuery(dom,
                                    "<folders> "
                                    "{for $a in doc($inputDocument)//div[@id='sb']//div[@class='content']//a[contains(@class, 'folder')]"
                                    "return "
                                    "<folder>"
                                    "  <url>{string($a/@href)}</url>"
                                    "  <title>{$a/span/text()}</title>"
                                    "  <imgUrl>{$a/img[contains(@class, 'folder-thumb')]/@src}</imgUrl>"
                                    "</folder>}"
                                    "</folders>\n");

    QDomDocument xqueryResults;
    xqueryResults.setContent(results);

    QDomNodeList folders = xqueryResults.elementsByTagName("folder");

    QString baseUrl = executeXQuery(dom, "string(doc($inputDocument)//div[contains(@class, 'info')]//h1//a/@href)");

    for (int i = 0; i < folders.count(); ++i)
    {
        EpisodeDirectory* ed = new EpisodeDirectory();

        QString url = folders.at(i).childNodes().at(0).toElement().text().trimmed();
        QString title = folders.at(i).childNodes().at(1).toElement().text().trimmed();
        QString imgUrl = folders.at(i).childNodes().at(2).toElement().text().trimmed();

        ed->url = QUrl(baseUrl + url);
        ed->title = QString("Katalog: " + title);
        ed->episodeImageFilepath = GetConfDir() + "/mythsvtplay/images/" + imgUrl.section('/', -1);
        ed->type = findType(dom);
        ed->position = items.count();

        items.insert(items.count(), ed);
    }
}

// not used yet, silly implementation
QList<QDateTime> findEpisodePubDates(const QDomDocument& dom)
{
    QList<QDateTime> dateList;

    dateList.push_back(QDateTime::currentDateTime());

    return dateList;
}

IProgramItem* EpisodeListBuilder::parseEpisodeDoc(const QDomDocument& dom)
{
    Episode* episode = new Episode;

    episode->type = findType(dom);

    episode->title = findTitle(dom);

    if (episode->title == "SVT Play")
    {
        delete episode;
        return NULL;
    }

    episode->description = findDescription(dom);

    if (episode->description.simplified() == "")
    {
        episode->description = "Ingen beskrivning tillgänglig.";
    }

    episode->publishedDate = QDateTime::currentDateTime();
    episode->availableUntilDate = QString::fromUtf8("Tillgänglig t.o.m. ") + findAvailableUntilDate(dom) + ".";
    episode->mediaUrls = findMediaUrls(dom);

    QUrl episodeImageUrl = findEpisodeImageUrl(dom);
    downloadImage(episodeImageUrl);
    episode->episodeImageFilepath = GetConfDir() + "/mythsvtplay/images/" + episodeImageUrl.path().section('/', -1);

    if (episode->mediaUrls["wmv"].path().contains("asx") ||
        episode->mediaUrls["wmv"].toString().contains("geoip"))
    {
        episode->urlIsPlaylist = true;
    }
    else
    {
        episode->urlIsPlaylist = false;
    }

    return episode;
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

QMap<QString, QUrl> findMediaUrls(const QDomDocument& dom)
{
    QMap<QString, QUrl> mediaUrls;

    mediaUrls["wmv"] = QUrl(executeXQuery(dom, "string(doc($inputDocument)//a[(contains(@href, '.asx') or contains(@href, '.wmv'))][1]/@href)"));

    mediaUrls["flv"] = QUrl(executeXQuery(dom, "string(doc($inputDocument)//a[(contains(@href, '.flv'))][1]/@href)"));

    QRegExp rx("url:([a-zA-Z0-9:/\\._\\-]*),bitrate:([0-9]*)");
    QString flashVars = executeXQuery(dom, "string((doc($inputDocument)//param[@name='flashvars'])[1]/@value)");
    int pos = 0;

    while ((pos = rx.indexIn(flashVars, pos)) != -1)
    {
        QString bitrate = rx.cap(2);
        QString url = rx.cap(1);

        if (bitrate.toInt() <= Settings::GetMaxBitrate())
        {
            if (url.contains("rtmps") || url.contains("rtmpe"))
            {
                mediaUrls["rtmps"] = QUrl(url);
            }
            else
            {
                mediaUrls["rtmp"] = QUrl(url);
            }

            break;
        }

        pos += rx.matchedLength();
    }

    if (pos == -1 && mediaUrls["rtmps"].isEmpty())
    {
        mediaUrls["rtmps"] = QUrl(executeXQuery(dom, "string(doc($inputDocument)//a[(contains(@href, 'rtmps://') or contains(@href, 'rtmpe://'))][1]/@href)"));
    }

    if (pos == -1 && mediaUrls["rtmp"].isEmpty())
    {
        mediaUrls["rtmp"] = QUrl(executeXQuery(dom, "string(doc($inputDocument)//a[(contains(@href, 'rtmp://'))][1]/@href)"));
    }

    return mediaUrls;
}

QUrl findEpisodeImageUrl(const QDomDocument& dom)
{
    QString flashvars = executeXQuery(dom, "string((doc($inputDocument)//param[@name='flashvars'])[1]/@value)");

    int index = flashvars.indexOf("background=") + 11;

    QString imageUrl = QString("");

    if (index != -1)
    {
        QString s1 = flashvars.mid(index);
        imageUrl = s1.left(s1.indexOf("&amp;"));
    }
    else
    {
        std::cerr << "No episode image found." << std::endl;
    }

    return QUrl(imageUrl);
}


QString findType(const QDomDocument& dom)
{
    QString type = executeXQuery(dom, "doc($inputDocument)//div[@id='sb']//ul[@class='navigation playerbrowser']//li[@class='selected']//a/text()");

    return type;
}

QString findNextPageQueryString(const QDomDocument& dom)
{
    QString nextImageUrl = executeXQuery(dom, "string(doc($inputDocument)//div[@id='sb']//ul[@class='pagination program']/li[contains(@class, 'next')]/a/img/@src)");

    if (nextImageUrl.contains("disabled"))
    {
        return "";
    }

    QString string = executeXQuery(dom, "string(doc($inputDocument)//div[@id='sb']//ul[@class='pagination program']/li[((@class='next ') or (@class='next'))]/a/@href)");

    return string;
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

QDomDocument* parseSvtPlayReply(QNetworkReply* reply)
{
    QString replyData = QString::fromUtf8(reply->readAll());
    replyData.replace(QRegExp("<video(.*)controls></video>"), "");

    QDomDocument* doc = new QDomDocument();

    QString error;
    int errorColumn;
    int errorLine;

    if (!doc->setContent(replyData, &error, &errorLine, &errorColumn))
    {
        std::cerr << "Failed to parse: " << std::endl;
        std::cerr << reply->url().toString().toStdString() << std::endl;

        std::cerr << "Error: " << error.toStdString() << std::endl;
        std::cerr << "Line: " << errorLine << ", Column: " << errorColumn << std::endl;

        delete doc;

        return NULL;
    }
    else
    {
        return doc;
    }
}

