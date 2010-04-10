#ifndef PROGRAMLISTBUILDER_H
#define PROGRAMLISTBUILDER_H

#include <QObject>
#include <QNetworkAccessManager>

class QUrl;
class QDomDocument;

class MythGenericTree;

class Program;

class ProgramListBuilder : public QObject
{
    Q_OBJECT

public:
    ProgramListBuilder();
    ~ProgramListBuilder();

    void buildProgramList();
    void abort();

public slots:
    void onDownloadFinished(QNetworkReply*);

signals:
    void programListBuilt(const QList<Program*>& programs);

    void numberOfProgramsFound(int count);
    void numberOfProgramsComplete(int count);
    void programComplete(const QString& title);

private:

    enum DOWNLOAD_STATE { INITIAL, POPULATE_PROGRAM_INFO, DOWNLOAD_IMAGES };

    void downloadXmlDocument(const QUrl& url);
    void downloadImage(Program* program);

    void doDownloadFsm();

    void fillProgramTitlesAndUrls(const QDomDocument& aoListDocument);
    void fillOtherProgramInfo(Program* program, const QDomDocument& programDocument);

    DOWNLOAD_STATE state_;

    bool aborted_;

    QNetworkAccessManager programListDownloader_;
    QList<QNetworkReply*> pendingReplies_;
    QList<QNetworkReply*> readyReplies_;

    QNetworkAccessManager imageDownloader_;
    QList<QNetworkReply*> imageDownloadQueue_;

    int programsComplete_;

    QList<Program*> programs_;
};

#endif // PROGRAMTREEBUILDER_H
