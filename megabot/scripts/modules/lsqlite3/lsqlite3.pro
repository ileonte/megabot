include(../../../../main.pri)

QT -= core gui
TEMPLATE = lib
INCLUDEPATH += . ../../../../libs/sqlite3 ../../../../libs/lua
LIBS += ../../../../libs/sqlite3/libsqlite3.a -L$$MB_BUILD_DIR/build/lib -llua
CONFIG += dynamiclib
TARGET = $$MB_BUILD_DIR/share/lib/lua/lsqlite3

SOURCES = lsqlite3.c
