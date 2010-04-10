#ifndef PROGRAM_H
#define PROGRAM_H

#include <QString>
#include <QUrl>

#include <iostream>

#include "Episode.h"

struct Program
{
    QString firstLetter;
    QString title;
    QString description;
    QString category;

    QString logoFilepath;

    QList<Episode*> episodes;

    QString link;
    QUrl rssUrl;
    QUrl logoUrl;
};

Q_DECLARE_METATYPE(Program)
Q_DECLARE_METATYPE(Program*)

QDataStream& operator<<(QDataStream& s, const Program& p);
QDataStream& operator>>(QDataStream& s, Program& p);

#endif // PROGRAM_H
