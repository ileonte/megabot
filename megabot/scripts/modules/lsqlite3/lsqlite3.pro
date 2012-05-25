include(../../../../main.pri)

QT -= core gui
TEMPLATE = lib
INCLUDEPATH += . ../../../../libs/sqlite3 ../../../../libs/lua
LIBS += ../../../../libs/sqlite3/libsqlite3.a -L../../../../libs/lua -llua
QMAKE_LFLAGS += -Wl,-rpath,$$MB_BASE_DIR/libs/lua
CONFIG += dynamiclib
TARGET = ../lsqlite3

SOURCES = lsqlite3.c
