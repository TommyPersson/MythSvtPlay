include ( ../../mythconfig.mak )
include ( ../../settings.pro )
QT += xml
QMAKE_STRIP = echo
TARGET = themenop
TEMPLATE = app
CONFIG -= qt \
    moc
QMAKE_COPY_DIR = sh \
    ../../cpsvndir

defaultfiles.path = $${PREFIX}/share/mythtv/themes/default/mythsvtplay
defaultfiles.files = default/*.xml

images.path = $${PREFIX}/share/mythtv/themes/default/mythsvtplay/images
images.files = default/images/*.png

widefiles.path = $${PREFIX}/share/mythtv/themes/default-wide
widefiles.files = default-wide/*.xml \
    default-wide/images/*.png

menufiles.path = $${PREFIX}/share/mythtv/
menufiles.files = menus/*.xml

INSTALLS += defaultfiles \
    images \
    widefiles \
    menufiles

# Input
SOURCES += ../../themedummy.c

OTHER_FILES += default/svtplay-ui.xml
