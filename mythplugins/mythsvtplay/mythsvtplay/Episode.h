#ifndef EPISODE_H
#define EPISODE_H

#include <QString>
#include <QDate>
#include <QUrl>
#include <QMetaType>

struct Episode
{
    QString title;
    QString description;
    QString type;

    QDateTime publishedDate;
    QString availableUntilDate;

    QUrl mediaUrl;
    bool urlIsPlaylist;
    QString episodeImageFilepath;

    int position;
};

bool comparePosition(Episode* l, Episode* r);

Q_DECLARE_METATYPE(Episode)
Q_DECLARE_METATYPE(Episode*)

#endif // EPISODE_H
