#ifndef MYTHSVTPLAY_H
#define MYTHSVTPLAY_H

#include <mythtv/uitypes.h>
#include <mythtv/uilistbtntype.h>
#include <mythtv/xmlparse.h>
#include <mythtv/mythdialogs.h>


class MythSvtPlay : virtual public MythThemedDialog
{


public:

    MythSvtPlay(MythMainWindow *parent, QString windowName,
           QString themeFilename, const char *name = 0);
    ~MythSvtPlay();

};

#endif // MYTHSVTPLAY_H
