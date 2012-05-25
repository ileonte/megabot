include(../main.pri)

QT       -= gui
QT       += core network xml

QMAKE_CXXFLAGS = -pipe -ggdb -O0

TARGET = megabot
TEMPLATE = app

INCLUDEPATH += ../libs/qxmpp/src/base ../libs/qxmpp/src/client ../libs/lua
LIBS += ../libs/qxmpp/src/libqxmpp.a -L$$MB_BASE_DIR/libs/lua -lm -llua -ldl -lpthread
QMAKE_LFLAGS += -Wl,-rpath,$$MB_BASE_DIR/libs/lua

SOURCES += main.cc \
    cxmpproom.cc \
    cxmppserver.cc \
    ctlpackets.cc \
    cscriptrunner.cc \
    cscriptcontroller.cc \
    cmegabot.cc \
    cluarunner.cc \
    utils.cc

HEADERS  += \
    main.h \
    utils.h \
    cxmppserver.h \
    cxmpproom.h \
    ctlpackets.h \
    cmegabot.h \
    cscriptcontroller.h \
    cscriptrunner.h \
    cluarunner.h

OTHER_FILES +=

RESOURCES +=
