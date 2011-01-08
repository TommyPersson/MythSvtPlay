include ( ../../mythconfig.mak )
include ( ../../settings.pro )
include ( ../../programs-libs.pro )
TEMPLATE = lib
CONFIG += plugin \
    thread
TARGET = mythsvtplay
target.path = $${LIBDIR}/mythtv/plugins
INSTALLS += target
QT += sql \
      xml \
      network \
      xmlpatterns

# Input
HEADERS += MainWindow.h \
    ProgramWindow.h \
    ProgressDialog.h \
    ProgramListBuilder.h \
    EpisodeListBuilder.h \
    Episode.h \
    Program.h \
    ImageLoader.h \
    ProgramListCache.h \
    IMediaPlayer.h \
    PlainMediaPlayer.h \
    RtmpMediaPlayer.h \
    FavoritesStore.h \
    Settings.h
SOURCES += main.cpp \
    MainWindow.cpp \
    ProgramWindow.cpp \
    ProgressDialog.cpp \
    ProgramListBuilder.cpp \
    EpisodeListBuilder.cpp \
    ImageLoader.cpp \
    ProgramListCache.cpp \
    Program.cpp \
    Episode.cpp \
    PlainMediaPlayer.cpp \
    RtmpMediaPlayer.cpp \
    FavoritesStore.cpp \
    Settings.cpp

macx:QMAKE_LFLAGS += -flat_namespace \
    -undefined \
    suppress
use_hidesyms:QMAKE_CXXFLAGS += -fvisibility=hidden
OTHER_FILES += 
include ( ../../libs-targetfix.pro )
