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
    ProgramListBuilder.h \
    EpisodeListBuilder.h \
    Episode.h \
    Program.h \
    ProgramWindow.h \
    MediaPlayer.h \
    ImageLoader.h \
    ProgressDialog.h \
    ProgramListCache.h \
    IMediaPlayer.h \
    RtmpMediaPlayer.h \
    FavoritesStore.h
SOURCES += main.cpp \
    MainWindow.cpp \
    ProgramListBuilder.cpp \
    EpisodeListBuilder.cpp \
    ProgramWindow.cpp \
    MediaPlayer.cpp \
    ImageLoader.cpp \
    ProgressDialog.cpp \
    ProgramListCache.cpp \
    Program.cpp \
    Episode.cpp \
    RtmpMediaPlayer.cpp \
    FavoritesStore.cpp

macx:QMAKE_LFLAGS += -flat_namespace \
    -undefined \
    suppress
use_hidesyms:QMAKE_CXXFLAGS += -fvisibility=hidden
OTHER_FILES += 
include ( ../../libs-targetfix.pro )
