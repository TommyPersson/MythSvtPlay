#ifndef PROGRAM_H
#define PROGRAM_H

#include <QString>
#include <QUrl>

#include "Episode.h"

struct Program
{
    QString title;
    QString description;

    QString logoFilepath;

    QList<Episode*> episodes;
};

#endif // PROGRAM_H
