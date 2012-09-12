include(../main.pri)

QT       -= gui
QT       += core network xml

QMAKE_CXXFLAGS = -pipe -ggdb -O0

TARGET = $$MB_BUILD_DIR/bin/megabot.bin
TEMPLATE = app

INCLUDEPATH += ../libs/qxmpp/src/base ../libs/qxmpp/src/client ../libs/lua ../libs/qt-json
LIBS += ../libs/qxmpp/src/libqxmpp.a ../libs/qt-json/libqtjson.a -L$$MB_BASE_DIR/libs/lua -lm -llua -ldl -lpthread
QMAKE_LFLAGS += -Wl,-rpath,$$MB_BASE_DIR/libs/lua

SOURCES += main.cc \
    cxmpproom.cc \
    cxmppserver.cc \
    ctlpackets.cc \
    cscriptrunner.cc \
    cscriptcontroller.cc \
    cmegabot.cc \
    cluarunner.cc \
    utils.cc \
    clogger.cc

HEADERS  += \
    main.h \
    utils.h \
    cxmppserver.h \
    cxmpproom.h \
    ctlpackets.h \
    cmegabot.h \
    cscriptcontroller.h \
    cscriptrunner.h \
    cluarunner.h \
    clogger.h

OTHER_FILES +=

RESOURCES +=

copyToDestdir($$MB_BUILD_DIR/share, scripts/*.lua)
