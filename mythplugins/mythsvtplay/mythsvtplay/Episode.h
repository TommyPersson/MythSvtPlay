#ifndef EPISODE_H
#define EPISODE_H

#include <QString>
#include <QDate>
#include <QUrl>

struct Episode
{
    QString title;
    QString description;

    QDate publishedDate;
    QDate availableUntilDate;

    QUrl asxUrl;
    QUrl episodeImageUrl;
};

#endif // EPISODE_H
