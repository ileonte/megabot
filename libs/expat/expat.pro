QT -= core gui
TEMPLATE = lib
INCLUDEPATH += .
QMAKE_CFLAGS_RELEASE += -DHAVE_EXPAT_CONFIG_H
QMAKE_CFLAGS_DEBUG += -DHAVE_EXPAT_CONFIG_H
CONFIG += staticlib
TARGET = expat

SOURCES = xmlparse.c \
          xmlrole.c \
          xmltok.c \
          xmltok_impl.c \
          xmltok_ns.c

HEADERS = expat.h
