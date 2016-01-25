#ifndef __CLUALIBLOADER_H_INCLUDED__
#define __CLUALIBLOADER_H_INCLUDED__

#include <QObject>
#include <QLibrary>
#include <QString>

#define LUA_TNONE (-1)
#define LUA_TNIL 0
#define LUA_TBOOLEAN 1
#define LUA_TLIGHTUSERDATA 2
#define LUA_TNUMBER 3
#define LUA_TSTRING 4
#define LUA_TTABLE 5
#define LUA_TFUNCTION 6
#define LUA_TUSERDATA 7
#define LUA_TTHREAD 8

#define LUA_REGISTRYINDEX (-10000)
#define LUA_ENVIRONINDEX (-10001)
#define LUA_GLOBALSINDEX (-10002)

class CLuaLibLoader : public QObject
{
	Q_OBJECT

public:
	typedef void lua_State;
	typedef int (*lua_CFunction)(lua_State *L);

	/*
	 * NOTE: these assume default Lua config. Change if you know your Lua
	 * library has been
	 * compiled with other values
	 */
	typedef double lua_Number;
	typedef ptrdiff_t lua_Integer;

	typedef void (*pfn_close)(lua_State *L);
	pfn_close lua_close;

	typedef int (*pfn_pcall)(lua_State *L, int nargs, int nret, int msgh);
	pfn_pcall lua_pcall;

	typedef int (*pfn_isfunction)(lua_State *L, int index);
	pfn_isfunction lua_isfunction_;
	int lua_isfunction(lua_State *L, int index) const
	{
		if (lua_isfunction_) {
			return lua_isfunction_(L, index);
		}
		return lua_type(L, index) == LUA_TFUNCTION;
	}

	typedef void (*pfn_setfield)(lua_State *L, int index, const char *k);
	pfn_setfield lua_setfield;

	typedef void (*pfn_getfield)(lua_State *L, int index, const char *k);
	pfn_getfield lua_getfield;

	typedef void (*pfn_getglobal)(lua_State *L, const char *name);
	pfn_getglobal lua_getglobal_;
	void lua_getglobal(lua_State *L, const char *name) const
	{
		if (lua_getglobal_) {
			lua_getglobal_(L, name);
			return;
		}
		lua_getfield(L, LUA_GLOBALSINDEX, name);
	}

	typedef void (*pfn_setglobal)(lua_State *L, const char *name);
	pfn_setglobal lua_setglobal_;
	void lua_setglobal(lua_State *L, const char *name) const
	{
		if (lua_setglobal_) {
			lua_setglobal_(L, name);
			return;
		}
		lua_setfield(L, LUA_GLOBALSINDEX, name);
	}

	typedef void (*pfn_createtable)(lua_State *L, int narr, int nrec);
	pfn_createtable lua_createtable;
	void lua_newtable(lua_State *L) const { lua_createtable(L, 0, 0); }
	typedef void (*pfn_pushstring)(lua_State *L, const char *s);
	pfn_pushstring lua_pushstring_;
	void lua_pushstring(lua_State *L, const char *s) const { lua_pushstring_(L, s); }
	void lua_pushstring(lua_State *L, const QString &s) const { lua_pushstring_(L, s.toUtf8().data()); }

	typedef void (*pfn_pushnumber)(lua_State *L, lua_Number n);
	pfn_pushnumber lua_pushnumber;

	typedef void (*pfn_pushinteger)(lua_State *L, lua_Integer n);
	pfn_pushinteger lua_pushinteger;

	typedef void (*pfn_settable)(lua_State *L, int index);
	pfn_settable lua_settable;

	typedef int (*pfn_gettop)(lua_State *L);
	pfn_gettop lua_gettop;

	typedef const char *(*pfn_tolstring)(lua_State *L, int index, size_t *len);
	pfn_tolstring lua_tolstring;
	const char *lua_tostring(lua_State *L, int index) const { return lua_tolstring(L, index, 0); }

	typedef int (*pfn_toboolean)(lua_State *L, int index);
	pfn_toboolean lua_toboolean;

	typedef lua_Integer (*pfn_tointeger)(lua_State *L, int index);
	pfn_tointeger lua_tointeger;

	typedef lua_Number (*pfn_tonumber)(lua_State *L, int idx);
	pfn_tonumber lua_tonumber;

	typedef const char *(*pfn_L_checklstring)(lua_State *L, int arg, size_t *l);
	pfn_L_checklstring luaL_checklstring;
	const char *luaL_checkstring(lua_State *L, int arg) const { return luaL_checklstring(L, arg, 0); }

	typedef int (*pfn_type)(lua_State *L, int index);
	pfn_type lua_type;

	typedef void (*pfn_pushnil)(lua_State *L);
	pfn_pushnil lua_pushnil;

	typedef int (*pfn_next)(lua_State *L, int index);
	pfn_next lua_next;

	typedef void (*pfn_settop)(lua_State *L, int index);
	pfn_settop lua_settop;

	typedef void (*pfn_pop)(lua_State *L, int n);
	pfn_pop lua_pop_;
	void lua_pop(lua_State *L, int n) const
	{
		if (lua_pop_) {
			lua_pop_(L, n);
			return;
		}
		lua_settop(L, -(n)-1);
	}

	typedef int (*pfn_L_error)(lua_State *L, const char *fmt, ...);
	pfn_L_error luaL_error;

	typedef void (*pfn_pushboolean)(lua_State *L, int b);
	pfn_pushboolean lua_pushboolean;

	typedef const char *(*pfn_pushlstring)(lua_State *L, const char *s, size_t len);
	pfn_pushlstring lua_pushlstring;

	typedef int (*pfn_isstring)(lua_State *L, int index);
	pfn_isstring lua_isstring;

	typedef int (*pfn_isnumber)(lua_State *L, int index);
	pfn_isnumber lua_isnumber;

	typedef int (*pfn_L_loadfile)(lua_State *L, const char *filename);
	pfn_L_loadfile luaL_loadfile_;
	int luaL_loadfile(lua_State *L, const char *filename) const { return luaL_loadfile_(L, filename); }
	int luaL_loadfile(lua_State *L, const QString &filename) const { return luaL_loadfile_(L, filename.toUtf8().data()); }

	typedef void (*pfn_L_openlibs)(lua_State *L);
	pfn_L_openlibs luaL_openlibs;

	typedef void (*pfn_pushcclosure)(lua_State *L, lua_CFunction f, int n);
	pfn_pushcclosure lua_pushcclosure;

	typedef void (*pfn_pushcfunction)(lua_State *L, lua_CFunction f);
	pfn_pushcfunction lua_pushcfunction_;
	void lua_pushcfunction(lua_State *L, lua_CFunction f) const
	{
		if (lua_pushcfunction_) {
			lua_pushcfunction_(L, f);
			return;
		}
		lua_pushcclosure(L, f, 0);
	}

	void lua_register(lua_State *L, const char *name, lua_CFunction f) const
	{
		lua_pushcfunction(L, f);
		lua_setglobal(L, name);
	}

	typedef lua_State *(*pfn_L_newstate)(void);
	pfn_L_newstate luaL_newstate;

private:
	QLibrary *m_lib;
	bool m_valid;

public:
	CLuaLibLoader(const QString &libPath, QObject *parent = 0);
	virtual ~CLuaLibLoader() {}
	bool isValid() const { return m_valid; }
	QString error() const { return m_lib->errorString(); }
};

#endif // __CLUALIBLOADER_H_INCLUDED__
