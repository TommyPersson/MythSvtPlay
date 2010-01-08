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

    QDate publishedDate;
    QDate availableUntilDate;

    QUrl mediaUrl;
    bool urlIsPlaylist;
    QUrl episodeImageUrl;
};

Q_DECLARE_METATYPE(Episode)
Q_DECLARE_METATYPE(Episode*)

#endif // EPISODE_H
