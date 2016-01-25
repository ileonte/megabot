#ifndef __CLUARUNNER_H_INCLUDED__
#define __CLUARUNNER_H_INCLUDED__

#include "clualibloader.h"
#include "cscriptrunner.h"
#include "utils.h"

#define SERIALIZE_MAX_DEPTH      30
#define SERIALIZE_BUFF_INCREMENT 16384

class CLuaRunner : public CScriptRunnerBase
{
	Q_OBJECT

private:
	QStringMap m_luaConfig;
	QStringMap m_scriptEnv;
	CLuaLibLoader *m_luaLib;
	CLuaLibLoader::lua_State *L;

	bool safeCallLua( int nargs, int nresults );
	QString getLuaError();

protected:
	virtual void onRoomConfigPacket( const CRoomConfigPacket &pkt );
	virtual void onRoomMessagePacket( const CRoomMessagePacket &pkt );
	virtual void onRoomPresencePacket( const CRoomPresencePacket &pkt );

	virtual void onNetworkRequestFinished( bool allOk, const QString &name, const QString &url, const QByteArray &data );

	virtual void onTimerTimeout( const QString &name );

public:
	CLuaRunner( const QString &handle, const QString &name, int fd, const QVariantMap &extraConfig, QObject *parent = 0 );
	virtual ~CLuaRunner();

	const CLuaLibLoader *luaLib() const { return m_luaLib; }

	virtual bool setupScript();
};

#endif // __CLUARUNNER_H_INCLUDED__
