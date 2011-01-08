#include "Episode.h"
#include "Program.h"

#include <QStringList>

bool comparePosition(IProgramItem* l, IProgramItem* r)
{
    return l->position < r->position;
}

Program* EpisodeDirectory::createProgram(const Program& p)
{
    Program* prog = new Program();

    prog->category = p.category;
    prog->description = p.description;
    prog->logoFilepath = p.logoFilepath;
    prog->logoUrl = p.logoUrl;
    prog->link = url.toString();

    QString directoryName = title.split(":").at(1).trimmed();

    prog->title = p.title + ": " + directoryName;

    QList<QPair<QString,QString> > typeLinks;
    typeLinks.append(QPair<QString, QString>(directoryName, url.toString()));
    prog->episodeTypeLinks = typeLinks;

    return prog;
}
