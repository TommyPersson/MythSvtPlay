#ifndef PROGRAM_H
#define PROGRAM_H

#include <QString>
#include <QUrl>

#include "Episode.h"

struct Program
{
    QString title;
    QString description;

    QUrl showLogoUrl;

    QList<Episode*> episodes;
};

#endif // PROGRAM_H
