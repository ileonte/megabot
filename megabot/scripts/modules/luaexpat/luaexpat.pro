include(../../../../main.pri)

QT -= core gui
TEMPLATE = lib
INCLUDEPATH += . ../../../../libs/lua ../../../../libs/libexpat
LIBS += ../../../../libs/expat/libexpat.a -L$$MB_BASE_DIR/libs/lua -llua
CONFIG += dynamiclib
TARGET = $$MB_BUILD_DIR/share/lib/lua/lxp
QMAKE_LFLAGS += -Wl,-version-script=vscript

SOURCES = lxplib.c

HEADERS = lxplib.h
