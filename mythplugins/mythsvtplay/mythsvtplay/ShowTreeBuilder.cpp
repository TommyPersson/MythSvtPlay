#include "ShowTreeBuilder.h"

#include <QUrl>
#include <QNetworkRequest>
#include <QNetworkReply>

#include <QDomDocument>
#include <QDomNode>
#include <QDomNodeList>
#include <QDomNamedNodeMap>

#include <mythtv/libmythui/mythgenerictree.h>

#include <iostream>

ShowTreeBuilder::ShowTreeBuilder()
{
    QObject::connect(&manager_, SIGNAL(finished(QNetworkReply*)),
                     this, SLOT(onDownloadFinished(QNetworkReply*)));

    root_ = new MythGenericTree("root-node");
}

void ShowTreeBuilder::buildTree()
{
    std::cerr << "buildTree() called" << std::endl;
    downloadXmlDocument(QUrl("http://svtplay.se/alfabetisk"));
}

void ShowTreeBuilder::downloadXmlDocument(const QUrl& url)
{
    QNetworkReply* reply = manager_.get(QNetworkRequest(url));
}

void ShowTreeBuilder::onDownloadFinished(QNetworkReply* reply)
{
    std::cerr << "onDownloadFinished() called" << std::endl;

    if (reply->url() == QUrl("http://svtplay.se/alfabetisk"))
    {
        MythGenericTree* alphabeticTree = parseAlphabetic(reply->readAll());

        root_->addNode(alphabeticTree);
    }

    if (true)
    {
        MythGenericTree* cathegoryTree = new MythGenericTree(QString::fromUtf8("Kategorier"));

        root_->addNode(cathegoryTree);
    }

    reply->deleteLater();

    emit treeBuilt(root_);
}

MythGenericTree* ShowTreeBuilder::parseAlphabetic(const QByteArray& bytes)
{

    QDomDocument doc;
    if (doc.setContent(bytes))
    {
        std::cerr << "Yay! Parsed!" << std::endl;
    }
    else
    {
        std::cerr << "Nay! Not Parsed!" << std::endl;
        return 0;
    }

    QDomNodeList nodes = doc.elementsByTagName("ul");

    MythGenericTree* tree = new MythGenericTree(QString::fromUtf8("Program A-Ö"));
    MythGenericTree* currentLetter = 0;

    for (int i = 0; i < nodes.count(); ++i)
    {
        QDomNode node = nodes.at(i);
        QDomNamedNodeMap attributes = node.attributes();
        if (attributes.contains("class"))
        {
            QString attributeValue = attributes.namedItem("class").toAttr().value();
            if (attributeValue.contains("leter-list"))
            {
                QDomNodeList listNodes = node.childNodes();
                for (int j = 0; j < listNodes.count(); ++j)
                {
                    QDomNode child = listNodes.at(j).firstChild();

                    if (child.nodeName() == "h2")
                    {
                        QString text = child.toElement().text();
                        currentLetter = new MythGenericTree(text);
                        tree->addNode(currentLetter);

                        std::wcerr << "New section: " << text.toStdWString() << std::endl;
                    }

                    if (child.nodeName() == "a")
                    {
                        QString link = child.attributes().namedItem("href").toAttr().value();
                        QString show = child.toElement().text();

                        MythGenericTree* showNode = new MythGenericTree(show);
                        showNode->SetData(QVariant(link));

                        currentLetter->addNode(showNode);

                        std::wcerr << "Program: " << show.toStdWString() << " (" << link.toStdWString() << ")" << std::endl;
                    }
                }
            }
        }
    }

    return tree;
}
