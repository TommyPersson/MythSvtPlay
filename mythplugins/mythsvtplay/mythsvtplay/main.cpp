#ifndef MAIN_CPP
#define MAIN_CPP

using namespace std;

#include "mythsvtplay.h"
#include <mythtv/mythcontext.h>
#include <mythtv/mythdbcon.h>
#include <mythtv/lcddevice.h>
#include <mythtv/libmythui/myththemedmenu.h>

extern "C" {
    int mythplugin_init(const char *libversion);
    int mythplugin_run(void);
    int mythplugin_config(void);
}

int mythplugin_init(const char *libversion)
{
    if (!gContext->TestPopupVersion("mythsvtplay", libversion, MYTH_BINARY_VERSION))
        return -1;

    return 0;
}

int mythplugin_run (void)
{
    gContext->addCurrentLocation("mythsvtplay");

    MythHello hello(gContext->GetMainWindow(), "hello", "hello-");
    hello.exec();

    gContext->removeCurrentLocation();

    return 1;
}

int mythplugin_config (void) { return 0; }

#endif
