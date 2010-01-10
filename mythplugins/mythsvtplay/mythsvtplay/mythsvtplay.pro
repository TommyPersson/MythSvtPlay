include ( ../../mythconfig.mak )
include ( ../../settings.pro )
include ( ../../programs-libs.pro )
TEMPLATE = lib
CONFIG += plugin \
    thread
TARGET = mythsvtplay
target.path = $${LIBDIR}/mythtv/plugins
INSTALLS += target
QT += xml \
    sql \
    network

# Input
HEADERS += MainWindow.h \
    ShowTreeBuilder.h \
    EpisodeListBuilder.h \
    Episode.h \
    Program.h \
    ProgramWindow.h \
    MediaPlayer.h \
    ImageLoader.h \
    StreamDumper.h
SOURCES += main.cpp \
    MainWindow.cpp \
    ShowTreeBuilder.cpp \
    EpisodeListBuilder.cpp \
    ProgramWindow.cpp \
    MediaPlayer.cpp \
    ImageLoader.cpp \
    StreamDumper.cpp
QMAKE_CFLAGS += -fPIC
QMAKE_LFlAGS += -fPIC
macx:QMAKE_LFLAGS += -flat_namespace \
    -undefined \
    suppress
use_hidesyms:QMAKE_CXXFLAGS += -fvisibility=hidden
OTHER_FILES += 
include ( ../../libs-targetfix.pro )
