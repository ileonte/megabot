include(../../../main.pri)

copyToDestdir($$MB_BUILD_DIR/share/lib/lua, JSON.lua feedparser.lua lxp/lom.lua feedparser/XMLElement.lua feedparser/dateparser.lua feedparser/url.lua)

TEMPLATE = subdirs
SUBDIRS = luaexpat lsqlite3
CONFIG += ordered
