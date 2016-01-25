include(../main.pri)

QT -= gui
QT += core network xml

QMAKE_CXXFLAGS = -pipe -ggdb -O0 -DQXMPP_STATIC

TARGET = $$MB_BUILD_DIR/bin/megabot.bin
TEMPLATE = app

INCLUDEPATH += ../libs/qxmpp/src/base ../libs/qxmpp/src/client ../libs/lua ../libs/qt-json
LIBS += ../libs/qxmpp/src/libqxmpp.a -L$$MB_BASE_DIR/build/lib -lm -llua -ldl -lpthread
QMAKE_LFLAGS += -Wl,-rpath,$$MB_BASE_DIR/build/lib

SOURCES += main.cc \
    cxmpproom.cc \
    cxmppserver.cc \
    ctlpackets.cc \
    cscriptrunner.cc \
    cscriptcontroller.cc \
    cmegabot.cc \
    cluarunner.cc \
    utils.cc \
    clogger.cc \
    cjsonparser.cc \
    clualibloader.cc

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
    clogger.h \
    cjsonparser.h \
    clualibloader.h

OTHER_FILES +=

RESOURCES +=

copyToDestdir($$MB_BUILD_DIR/share, scripts/*.lua)
