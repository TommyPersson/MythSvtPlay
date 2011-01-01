#include "ProgramListBuilder.h"
#include "Program.h"

#include <QDir>
#include <QUrl>
#include <QMap>
#include <QPair>
#include <QNetworkRequest>
#include <QNetworkReply>

#include <QDomNode>
#include <QDomNodeList>
#include <QDomDocument>
#include <QDomNamedNodeMap>

#include <QTextStream>
#include <QRegExp>

#include <QBuffer>
#include <QXmlQuery>
#include <QXmlSerializer>

#include <mythtv/mythdirs.h>
#include <mythtv/libmythui/mythgenerictree.h>

#include <iostream>

static QUrl findProgramImageUrl(const QDomDocument& dom);
static QString findProgramDescription(const QDomDocument& dom);
static QString findProgramCategory(const QDomDocument& dom);
static QUrl findProgramRssFeed(const QDomDocument& dom);
static QList<QPair<QString, QString> > findProgramEpisodeTypeLinks(const QDomDocument& dom);
static QString executeXQuery(const QDomDocument& dom, const QString& query);

static QDomDocument* parseSvtPlayReply(QNetworkReply* reply);

ProgramListBuilder::ProgramListBuilder() :
        state_(INITIAL),
        aborted_(false),
        programsComplete_(0)
{
    QObject::connect(&programListDownloader_, SIGNAL(finished(QNetworkReply*)),
                     this, SLOT(onDownloadFinished(QNetworkReply*)));
}

ProgramListBuilder::~ProgramListBuilder()
{
    abort();
}

void ProgramListBuilder::buildProgramList()
{
    programsComplete_ = 0;
    state_ = INITIAL;

    downloadXmlDocument(QUrl("http://svtplay.se/alfabetisk"));
}

void ProgramListBuilder::abort()
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

    while (!programs_.isEmpty())
    {
        delete programs_.takeFirst();
    }
}

void ProgramListBuilder::downloadXmlDocument(const QUrl& url)
{
    QNetworkReply* reply = programListDownloader_.get(QNetworkRequest(url));
    pendingReplies_.push_front(reply);
}

void ProgramListBuilder::downloadImage(Program* program)
{
    QString filename = program->logoUrl.path().section('/', -1);

    QDir savelocation(GetConfDir() + "/mythsvtplay/images/");

    QFile savefile(savelocation.absolutePath() + "/" + filename);
    if (!savefile.exists())
    {
        QNetworkRequest request(program->logoUrl);
        request.setAttribute(QNetworkRequest::User, qVariantFromValue(program));

        QNetworkReply* showReply = programListDownloader_.get(request);

        pendingReplies_.push_back(showReply);
    }
    else
    {
        programsComplete_++;

        emit numberOfProgramsComplete(programsComplete_);
        emit programComplete(program->title);
    }

    program->logoFilepath = savelocation.absolutePath() + "/" + filename;
}

void ProgramListBuilder::onDownloadFinished(QNetworkReply* reply)
{
    pendingReplies_.removeOne(reply);
    readyReplies_.push_back(reply);

    doDownloadFsm();
}

void ProgramListBuilder::doDownloadFsm()
{
    switch (state_)
    {

    case INITIAL:
        {
            if (aborted_)
                return;

            QNetworkReply* reply = readyReplies_.takeFirst();

            QDomDocument* doc = parseSvtPlayReply(reply);

            reply->deleteLater();

            if (doc == NULL)
            {
                std::cerr << "Unable to parse reply: " << reply->url().path().toStdString() << std::endl;

                return;
            }

            fillProgramTitlesAndUrls(*doc);

            for (int i = 0; i < programs_.count(); ++i)
            {
                QUrl url("http://svtplay.se" + programs_[i]->link);
                QNetworkRequest request(url);
                request.setAttribute(QNetworkRequest::User, qVariantFromValue(programs_[i]));

                QNetworkReply* showReply = programListDownloader_.get(request);

                pendingReplies_.push_back(showReply);
            }
            emit numberOfProgramsComplete(0);
            emit numberOfProgramsFound(programs_.count());

            state_ = POPULATE_PROGRAM_INFO;

        }
        break;

    case POPULATE_PROGRAM_INFO:
        {
            if (aborted_)
                return;

            Program* peekProgram = readyReplies_.last()->request().attribute(QNetworkRequest::User).value<Program*>();;

            programsComplete_++;

            emit numberOfProgramsComplete(programsComplete_);
            emit programComplete(peekProgram->title);

            if (pendingReplies_.count() == 0)
            {
                while(!readyReplies_.empty())
                {
                    QNetworkReply* reply = readyReplies_.takeFirst();

                    Program* program = reply->request().attribute(QNetworkRequest::User).value<Program*>();

                    QDomDocument* doc = parseSvtPlayReply(reply);

                    if (doc == NULL)
                    {
                        reply->deleteLater();

                        failedDownloadCount_[program->title]++;

                        if (failedDownloadCount_[program->title] > 5)
                        {
                            std::cerr << "Giving up" << std::endl;

                            programs_.removeOne(program);
                        }
                        else
                        {
                            std::cerr << "Retrying (" << failedDownloadCount_[program->title] << ") ..."  << std::endl;

                            QUrl url("http://svtplay.se" + program->link);
                            QNetworkRequest request(url);
                            request.setAttribute(QNetworkRequest::User, qVariantFromValue(program));

                            QNetworkReply* showReply = programListDownloader_.get(request);

                            pendingReplies_.push_back(showReply);

                            return;
                        }
                    }
                    else
                    {
                        reply->deleteLater();

                        fillOtherProgramInfo(program, *doc);
                    }
                }

                programsComplete_ = 0;

                state_ = DOWNLOAD_IMAGES;

                for (int i = 0; i < programs_.count(); ++i)
                {
                    downloadImage(programs_[i]);
                }

                if (pendingReplies_.count() == 0)
                {
                    emit programListBuilt(programs_);

                    programs_.clear();
                }
            }
        }
        break;

    case DOWNLOAD_IMAGES:
        {
            if (aborted_)
                return;

            Program* peekProgram = readyReplies_.last()->request().attribute(QNetworkRequest::User).value<Program*>();;

            programsComplete_++;

            emit numberOfProgramsComplete(programsComplete_);
            emit programComplete(peekProgram->title);


            if (pendingReplies_.count() == 0)
            {
                while(!readyReplies_.empty())
                {
                    QNetworkReply* reply = readyReplies_.takeFirst();

                    Program* program = reply->request().attribute(QNetworkRequest::User).value<Program*>();

                    QUrl url = reply->url();
                    QString filename = url.path().section('/', -1);

                    QDir savelocation(GetConfDir() + "/mythsvtplay/images/");

                    QFile savefile(savelocation.absolutePath() + "/" + filename);

                    savefile.open(QIODevice::WriteOnly);
                    savefile.write(reply->readAll());
                    savefile.close();

                    program->logoFilepath = savelocation.absolutePath() + "/" + filename;
                }

                emit programListBuilt(programs_);

                programs_.clear();
            }

        }
        break;
    }
}

void ProgramListBuilder::fillProgramTitlesAndUrls(const QDomDocument& aoListDocument)
{
    QString results = executeXQuery(aoListDocument,
                                    "<programs>"
                                    "{for $a in doc($inputDocument)//ul[@class='leter-list']//a[contains(@href, '/t/')]"
                                    "return"
                                    "<program>"
                                    "  <link>{string($a/@href)}</link>"
                                    "  <title>{$a/text()}</title>"
                                    "</program>}"
                                    "</programs>\n");

    QDomDocument xqueryResults;
    xqueryResults.setContent(results);

    QDomNodeList programs = xqueryResults.elementsByTagName("program");

    for (int i = 0; i < programs.count(); ++i)
    {
        QString link = programs.at(i).childNodes().at(0).toElement().text();
        QString title = programs.at(i).childNodes().at(1).toElement().text();

        Program* program = new Program();

        program->firstLetter = title.at(0);
        program->link = link;
        program->title = title;

        programs_.push_back(program);
    }
}

void ProgramListBuilder::fillOtherProgramInfo(Program* program, const QDomDocument& programDocument)
{
    program->logoUrl = findProgramImageUrl(programDocument);
    program->description = findProgramDescription(programDocument);
    program->category = findProgramCategory(programDocument);
    program->rssUrl = findProgramRssFeed(programDocument);
    program->episodeTypeLinks = findProgramEpisodeTypeLinks(programDocument);
}

QString findProgramDescription(const QDomDocument& dom)
{
    QString description = executeXQuery(dom, "string(doc($inputDocument)//meta[@name='description']/@content)");

    return description;
}

QUrl findProgramImageUrl(const QDomDocument& dom)
{
    QString url = executeXQuery(dom, "string(doc($inputDocument)//meta[@property='og:image']/@content)");

    return QUrl(url);
}

QString findProgramCategory(const QDomDocument& dom)
{
    QString category = executeXQuery(dom, "doc($inputDocument)//span[@class='category']/a/text()");

    return category;
}

QUrl findProgramRssFeed(const QDomDocument& dom)
{
    QString rssFeed = executeXQuery(dom, "string(doc($inputDocument)//link[@rel='alternate' and contains(@href, 'expression=full')]/@href)");

    return QUrl(rssFeed);
}


QList<QPair<QString, QString> > findProgramEpisodeTypeLinks(const QDomDocument& dom)
{
    QList<QPair<QString, QString> > episodeTypeLinkList;

    QString results = executeXQuery(dom,
                                    "<links>"
                                    "{for $a in doc($inputDocument)//div[@id='sb']//ul[@class='navigation playerbrowser']//a[contains(@class, 'internal')]"
                                    "return"
                                    "<link>"
                                    "  <href>{string($a/@href)}</href>"
                                    "  <text>{$a/text()}</text>"
                                    "</link>}"
                                    "</links>\n");

    QDomDocument xqueryResults;
    xqueryResults.setContent(results);

    QDomNodeList links = xqueryResults.elementsByTagName("link");

    for (int i = 0; i < links.count(); ++i)
    {
        QString link = links.at(i).childNodes().at(0).toElement().text().trimmed();
        QString type = links.at(i).childNodes().at(1).toElement().text().trimmed();

        episodeTypeLinkList.push_back(QPair<QString, QString>(type, link));
    }

    return episodeTypeLinkList;
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
