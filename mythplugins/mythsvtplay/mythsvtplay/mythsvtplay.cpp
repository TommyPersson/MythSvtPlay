#ifndef MYTHHELLO_CPP
#define MYTHHELLO_CPP

/* QT includes */
#include <qnamespace.h>
#include <qstringlist.h>
#include <qapplication.h>
#include <qbuttongroup.h>

/* MythTV includes */
#include <mythtv/mythcontext.h>
#include <mythtv/mythdialogs.h>

#include "mythsvtplay.h"

using namespace std;


MythSvtPlay::MythSvtPlay(MythMainWindow *parent, QString windowName,
                           QString themeFilename, const char *name)
        : MythThemedDialog(parent, windowName, themeFilename, name)
{

}

MythSvtPlay::~MythSvtPlay() { }

#endif
