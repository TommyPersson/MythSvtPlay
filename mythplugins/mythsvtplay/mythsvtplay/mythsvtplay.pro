include ( ../../mythconfig.mak )
include ( ../../settings.pro )
TEMPLATE = lib
CONFIG += plugin \
    thread
TARGET = mythsvtplay
target.path = $${LIBDIR}/mythtv/plugins
INSTALLS += target
uifiles.path = $${PREFIX}/share/mythtv/themes/default
uifiles.files = svtplay-ui.xml
installfiles.path = $${PREFIX}/share/mythtv
installfiles.files = svtplay-ui.xml
INSTALLS += uifiles

# Input
HEADERS += mythsvtplay.h
SOURCES += main.cpp \
    mythsvtplay.cpp \
    mythsvtplay.cpp
macx:QMAKE_LFLAGS += -flat_namespace \
    -undefined \
    suppress
OTHER_FILES += svtplay-ui.xml
