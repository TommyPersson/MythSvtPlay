#ifndef PROGRAMTREEBUILDER_H
#define PROGRAMTREEBUILDER_H

#include <QObject>
#include <QNetworkAccessManager>

class QUrl;

class MythGenericTree;

class ShowTreeBuilder : public QObject
{
    Q_OBJECT

public:
    ShowTreeBuilder();

    void buildTree();

public slots:
    void onDownloadFinished(QNetworkReply*);

signals:
    void treeBuilt(MythGenericTree*);

private:
    void downloadXmlDocument(const QUrl& url);
    MythGenericTree* parseAlphabetic(const QByteArray& bytes);

    QNetworkAccessManager manager_;
    MythGenericTree* root_;
};

#endif // PROGRAMTREEBUILDER_H
