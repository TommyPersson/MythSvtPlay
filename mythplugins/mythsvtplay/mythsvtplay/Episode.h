#ifndef EPISODE_H
#define EPISODE_H

#include <QString>
#include <QDate>
#include <QUrl>
#include <QMetaType>
#include <QMap>

class Program;

struct IProgramItem
{
    QString title;
    QString description;
    QString type;

    QString episodeImageFilepath;

    int position;

    virtual ~IProgramItem() {};
};

struct Episode : public IProgramItem
{
    QDateTime publishedDate;
    QString availableUntilDate;

    QMap<QString, QUrl> mediaUrls;
    bool urlIsPlaylist;
};

struct EpisodeDirectory : public IProgramItem
{
    QUrl url;

    Program* createProgram(const Program& p);
};

bool comparePosition(IProgramItem* l, IProgramItem* r);

Q_DECLARE_METATYPE(IProgramItem*)
Q_DECLARE_METATYPE(IProgramItem)

#endif // EPISODE_H
