#include "Program.h"

QDataStream& operator<<(QDataStream& s, const Program& p)
{
    s << p.firstLetter
      << p.title
      << p.description
      << p.category
      << p.logoFilepath
      << p.link
      << p.rssUrl
      << p.logoUrl;

    return s;
}

QDataStream& operator>>(QDataStream& s, Program& p)
{
    s >> p.firstLetter
      >> p.title
      >> p.description
      >> p.category
      >> p.logoFilepath
      >> p.link
      >> p.rssUrl
      >> p.logoUrl;

    return s;
}
