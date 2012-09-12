#ifndef __CLUARUNNER_H_INCLUDED__
#define __CLUARUNNER_H_INCLUDED__

#include "cscriptrunner.h"

#include <utils.h>

extern "C" {
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
}

#define SERIALIZE_MAX_DEPTH      30
#define SERIALIZE_BUFF_INCREMENT 16384

class CLuaRunner : public CScriptRunnerBase
{
	Q_OBJECT

private:
	lua_State *L;

	bool safeCallLua( int nargs, int nresults );
	QString getLuaError();

protected:
	virtual void onRoomConfigPacket( const CRoomConfigPacket &pkt );
	virtual void onRoomMessagePacket( const CRoomMessagePacket &pkt );
	virtual void onRoomPresencePacket( const CRoomPresencePacket &pkt );

	virtual void onNetworkRequestFinished( bool allOk, const QString &name, const QString &url, const QByteArray &data );

	virtual void onTimerTimeout( const QString &name );

public:
	CLuaRunner( const QString &handle, const QString &name, int fd, QObject *parent = 0 );
	~CLuaRunner();

	virtual bool setupScript();

public slots:
	QString logHandle() { return m_handle; }
};

#endif // __CLUARUNNER_H_INCLUDED__
