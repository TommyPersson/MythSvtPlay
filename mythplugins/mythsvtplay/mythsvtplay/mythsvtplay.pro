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
    ProgramListCache.h
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
    Episode.cpp

macx:QMAKE_LFLAGS += -flat_namespace \
    -undefined \
    suppress
use_hidesyms:QMAKE_CXXFLAGS += -fvisibility=hidden
OTHER_FILES += 
include ( ../../libs-targetfix.pro )
