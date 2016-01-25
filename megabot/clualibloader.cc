#include "clualibloader.h"
#include "main.h"

#include <QDebug>

CLuaLibLoader::CLuaLibLoader(const QString &libPath, QObject *parent) : QObject(parent), m_valid(false)
{
	m_lib = new QLibrary(libPath, this);
	if (m_lib->load()) {
		lua_close = (pfn_close)m_lib->resolve("lua_close");
		lua_version = (pfn_version)m_lib->resolve("lua_version");
		lua_pcall_ = (pfn_pcall)m_lib->resolve("lua_pcall");
		lua_pcallk52_ = (pfn_pcallk52)m_lib->resolve("lua_pcallk");
		lua_pcallk53_ = (pfn_pcallk53)m_lib->resolve("lua_pcallk");
		lua_setfield = (pfn_setfield)m_lib->resolve("lua_setfield");
		lua_getfield = (pfn_getfield)m_lib->resolve("lua_getfield");
		lua_isfunction_ = (pfn_isfunction)m_lib->resolve("lua_isfunction");
		lua_getglobal_ = (pfn_getglobal)m_lib->resolve("lua_getglobal");
		lua_setglobal_ = (pfn_setglobal)m_lib->resolve("lua_setglobal");
		lua_createtable = (pfn_createtable)m_lib->resolve("lua_createtable");
		lua_pushstring_ = (pfn_pushstring)m_lib->resolve("lua_pushstring");
		lua_pushnumber = (pfn_pushnumber)m_lib->resolve("lua_pushnumber");
		lua_pushinteger = (pfn_pushinteger)m_lib->resolve("lua_pushinteger");
		lua_settable = (pfn_settable)m_lib->resolve("lua_settable");
		lua_gettop = (pfn_gettop)m_lib->resolve("lua_gettop");
		lua_tolstring = (pfn_tolstring)m_lib->resolve("lua_tolstring");
		lua_toboolean = (pfn_toboolean)m_lib->resolve("lua_toboolean");
		lua_tointeger_ = (pfn_tointeger)m_lib->resolve("lua_tointeger");
		lua_tointegerx = (pfn_tointegerx)m_lib->resolve("lua_tointegerx");
		lua_tonumber_ = (pfn_tonumber)m_lib->resolve("lua_tonumber");
		lua_tonumberx = (pfn_tonumberx)m_lib->resolve("lua_tonumberx");
		luaL_checklstring = (pfn_L_checklstring)m_lib->resolve("luaL_checklstring");
		lua_type = (pfn_type)m_lib->resolve("lua_type");
		lua_pushnil = (pfn_pushnil)m_lib->resolve("lua_pushnil");
		lua_next = (pfn_next)m_lib->resolve("lua_next");
		lua_settop = (pfn_settop)m_lib->resolve("lua_settop");
		lua_pop_ = (pfn_pop)m_lib->resolve("lua_pop");
		luaL_error = (pfn_L_error)m_lib->resolve("luaL_error");
		lua_pushboolean = (pfn_pushboolean)m_lib->resolve("lua_pushboolean");
		lua_pushlstring = (pfn_pushlstring)m_lib->resolve("lua_pushlstring");
		lua_isstring = (pfn_isstring)m_lib->resolve("lua_isstring");
		lua_isnumber = (pfn_isnumber)m_lib->resolve("lua_isnumber");
		luaL_loadfile_ = (pfn_L_loadfile)m_lib->resolve("luaL_loadfile");
		luaL_loadfilex_ = (pfn_L_loadfilex)m_lib->resolve("luaL_loadfilex");
		luaL_openlibs = (pfn_L_openlibs)m_lib->resolve("luaL_openlibs");
		lua_pushcclosure = (pfn_pushcclosure)m_lib->resolve("lua_pushcclosure");
		lua_pushcfunction_ = (pfn_pushcfunction)m_lib->resolve("lua_pushcfunction");
		luaL_newstate = (pfn_L_newstate)m_lib->resolve("luaL_newstate");

		m_valid = true;
	}
}
