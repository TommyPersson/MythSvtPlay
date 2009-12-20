include ( ../../mythconfig.mak )
include ( ../../settings.pro )
include ( ../../programs-libs.pro )
TEMPLATE = lib
CONFIG += plugin \
    thread
TARGET = mythsvtplay
target.path = $${LIBDIR}/mythtv/plugins
INSTALLS += target

# uifiles.path = $${PREFIX}/share/mythtv/themes/default
# uifiles.files = svtplay-ui.xml
# installfiles.path = $${PREFIX}/share/mythtv
# installfiles.files = svtplay-ui.xml
# INSTALLS += uifiles
QT += xml \
    sql \
    network

# Input
HEADERS += MainWindow.h \
    ShowTreeBuilder.h \
    EpisodeListBuilder.h \
    Episode.h \
    Show.h
SOURCES += main.cpp \
    MainWindow.cpp \
    ShowTreeBuilder.cpp \
    EpisodeListBuilder.cpp

QMAKE_CFLAGS += -fPIC
QMAKE_LFlAGS += -fPIC

macx:QMAKE_LFLAGS += -flat_namespace \
    -undefined \
    suppress
use_hidesyms:QMAKE_CXXFLAGS += -fvisibility=hidden
OTHER_FILES += 
include ( ../../libs-targetfix.pro )
