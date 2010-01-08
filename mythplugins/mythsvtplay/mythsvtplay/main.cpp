#ifndef MAIN_CPP
#define MAIN_CPP

#include <mythtv/mythpluginapi.h>
#include <mythtv/mythcontext.h>
#include <mythtv/mythdbcon.h>
#include <mythtv/mythversion.h>
#include <mythtv/lcddevice.h>
#include <mythtv/mythverbose.h>

#include <mythtv/libmythui/mythmainwindow.h>
#include <mythtv/libmythui/myththemedmenu.h>
#include <mythtv/libmythui/mythuihelper.h>

//#include "mythsvtplay.h"

#include <MainWindow.h>

int mythplugin_init(const char *libversion)
{
    if (!gContext->TestPopupVersion("mythsvtplay", libversion,
                                    MYTH_BINARY_VERSION))
    {
        VERBOSE(VB_IMPORTANT,
                QString("libmythsvtplay.so/main.o: binary version mismatch"));
        return -1;
    }

    return 0;
}

int mythplugin_run (void)
{
    MythScreenStack *mainStack = GetMythMainWindow()->GetMainStack();

    try
    {
        mainStack->AddScreen(new MainWindow(mainStack));
    }
    catch (...)
    {
        return -1;
    }

    return 0;

}

int mythplugin_config (void)
{
    return 0;
}

#endif
