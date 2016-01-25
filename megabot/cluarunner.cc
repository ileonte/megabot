#include "cscriptrunner.h"
#include "cluarunner.h"
#include "ctlpackets.h"

#include <QXmppPresence.h>

#include <qmath.h>
#include <ctype.h>

#define LUARUN (static_cast<CLuaRunner *>(botInstance->scriptRunner()))
#define LUALIB (static_cast<CLuaRunner *>(botInstance->scriptRunner())->luaLib())

CLuaRunner::CLuaRunner(const QString &handle, const QString &name, int fd, const QVariantMap &extraConfig, QObject *parent)
      : CScriptRunnerBase(handle, name, fd, extraConfig, parent), L(0)
{
	QString defLuaLib = fmt("%1/lib/liblua.so").arg(botInstance->basePath());
	QString luaLib = extraConfig.value("luaLib").toString();

	if (luaLib.isEmpty()) {
		m_luaConfig["luaLib"] = defLuaLib;
		m_scriptEnv["LUA_PATH"] = fmt("%1/share/lib/lua/?.lua").arg(botInstance->basePath());
		m_scriptEnv["LUA_CPATH"] = fmt("%1/share/lib/lua/lib?.so").arg(botInstance->basePath());
	} else {
		m_luaConfig["luaLib"] = luaLib;
		m_scriptEnv["LUA_PATH"] = fmt(";;%1/share/lib/lua/?.lua").arg(botInstance->basePath());
	}

	m_luaLib = new CLuaLibLoader(m_luaConfig["luaLib"], this);
}

CLuaRunner::~CLuaRunner()
{
	if ( L ) {
		m_luaLib->lua_getglobal( L, "handle_shutdown" );
		if ( m_luaLib->lua_isfunction( L, -1 ) ) {
			if ( !safeCallLua( 0, 0 ) )
				LOG( fmt( "safeCallLua(): %1" ).arg( getLuaError() ) );
		}
		m_luaLib->lua_close( L );
		L = 0;
	}
}

bool CLuaRunner::safeCallLua( int nargs, int nresults )
{
	int st = m_luaLib->lua_pcall( L, nargs, nresults, 0 );
	if ( st ) return false;
	return true;
}

QString CLuaRunner::getLuaError()
{
	return QString( m_luaLib->lua_tostring( L, -1 ) );
}

void CLuaRunner::onRoomConfigPacket( const CRoomConfigPacket &pkt )
{
	CScriptRunnerBase::onRoomConfigPacket( pkt );

	m_luaLib->lua_getglobal( L, "handle_room_config" );
	if ( !m_luaLib->lua_isfunction( L, -1 ) ) {
		LOG( "handle_room_config() is NOT a function" );
		botInstance->quit();
		return;
	}
	m_luaLib->lua_newtable( L );
	m_luaLib->lua_pushstring( L, "roomName" );
	m_luaLib->lua_pushstring( L, pkt.roomName().toUtf8().data() );
	m_luaLib->lua_settable( L, -3 );
	m_luaLib->lua_pushstring( L, "roomJid" );
	m_luaLib->lua_pushstring( L, pkt.roomJid().toUtf8().data() );
	m_luaLib->lua_settable( L, -3 );
	m_luaLib->lua_pushstring( L, "nickName" );
	m_luaLib->lua_pushstring( L, pkt.nickName().toUtf8().data() );
	m_luaLib->lua_settable( L, -3 );
	m_luaLib->lua_pushstring( L, "topic" );
	m_luaLib->lua_pushstring( L, pkt.topic().toUtf8().data() );
	m_luaLib->lua_settable( L, -3 );
	if ( !safeCallLua( 1, 0 ) )
		LOG( fmt( "safeCallLua(): %1" ).arg( getLuaError() ) );
}

void CLuaRunner::onRoomMessagePacket( const CRoomMessagePacket &pkt )
{
	CScriptRunnerBase::onRoomMessagePacket( pkt );

	if ( QXmppUtils::jidToResource( pkt.from() ) == m_nickName )
		return;

	m_luaLib->lua_getglobal( L, "handle_room_message" );
	if ( !m_luaLib->lua_isfunction( L, -1 ) ) {
		LOG( "handle_room_message() is NOT a function" );
		botInstance->quit();
		return;
	}
	m_luaLib->lua_newtable( L );
	m_luaLib->lua_pushstring( L, "from" );
	m_luaLib->lua_pushstring( L, pkt.from().toUtf8().data() );
	m_luaLib->lua_settable( L, -3 );
	m_luaLib->lua_pushstring( L, "body" );
	m_luaLib->lua_pushstring( L, pkt.body().toUtf8().data() );
	m_luaLib->lua_settable( L, -3 );
	m_luaLib->lua_pushstring( L, "nickName" );
	m_luaLib->lua_pushstring( L, QXmppUtils::jidToResource( pkt.from() ).toUtf8().data() );
	m_luaLib->lua_settable( L, -3 );
	m_luaLib->lua_pushstring( L, "serverTime" );
	m_luaLib->lua_pushnumber( L, pkt.serverTime() );
	m_luaLib->lua_settable( L, -3 );
	m_luaLib->lua_pushstring( L, "localTime" );
	m_luaLib->lua_pushnumber( L, pkt.localTime() );
	m_luaLib->lua_settable( L, -3 );
	if ( !safeCallLua( 1, 0 ) )
		LOG( fmt( "safeCallLua(): %1" ).arg( getLuaError() ) );
}

void CLuaRunner::onRoomPresencePacket( const CRoomPresencePacket &pkt )
{
	CScriptRunnerBase::onRoomPresencePacket( pkt );

	m_luaLib->lua_getglobal( L, "handle_room_presence" );
	if ( !m_luaLib->lua_isfunction( L, -1 ) ) {
		LOG( "handle_room_presence() is NOT a function" );
		botInstance->quit();
		return;
	}
	m_luaLib->lua_newtable( L );
	m_luaLib->lua_pushstring( L, "who" );
	m_luaLib->lua_pushstring( L, pkt.who().toUtf8().data() );
	m_luaLib->lua_settable( L, -3 );
	m_luaLib->lua_pushstring( L, "nickName" );
	m_luaLib->lua_pushstring( L, QXmppUtils::jidToResource( pkt.who() ).toUtf8().data() );
	m_luaLib->lua_settable( L, -3 );
	m_luaLib->lua_pushstring( L, "presence" );
	m_luaLib->lua_pushinteger( L, ( int )pkt.presenceType() );
	m_luaLib->lua_settable( L, -3 );
	m_luaLib->lua_pushstring( L, "statusType" );
	m_luaLib->lua_pushinteger( L, ( int )pkt.statusType() );
	m_luaLib->lua_settable( L, -3 );
	m_luaLib->lua_pushstring( L, "statusText" );
	m_luaLib->lua_pushstring( L, pkt.statusText().toUtf8().data() );
	m_luaLib->lua_settable( L, -3 );
	m_luaLib->lua_pushstring( L, "statusPriority" );
	m_luaLib->lua_pushinteger( L, pkt.statusPriority() );
	m_luaLib->lua_settable( L, -3 );
	if ( !safeCallLua( 1, 0 ) )
		LOG( fmt( "safeCallLua(): %1" ).arg( getLuaError() ) );
}

void CLuaRunner::onNetworkRequestFinished( bool allOk, const QString &name, const QString &url, const QByteArray &data )
{
	CScriptRunnerBase::onNetworkRequestFinished( allOk, name, url, data );

	m_luaLib->lua_getglobal( L, "handle_network_reply" );
	if ( !m_luaLib->lua_isfunction( L, -1 ) ) {
		LOG( "handle_network_reply is NOT a function" );
		botInstance->quit();
		return;
	}

	m_luaLib->lua_newtable( L );
	m_luaLib->lua_pushstring( L, "status" );
	m_luaLib->lua_pushboolean( L, ( int )allOk );
	m_luaLib->lua_settable( L, -3 );
	m_luaLib->lua_pushstring( L, "name" );
	m_luaLib->lua_pushstring( L, name.toUtf8().data() );
	m_luaLib->lua_settable( L, -3 );
	m_luaLib->lua_pushstring( L, "data" );
	m_luaLib->lua_pushlstring( L, data.data(), data.size() );
	m_luaLib->lua_settable( L, -3 );
	m_luaLib->lua_pushstring( L, "url" );
	m_luaLib->lua_pushstring( L, url.toUtf8().data() );
	m_luaLib->lua_settable( L, -3 );
	if ( !safeCallLua( 1, 0 ) )
		LOG( fmt( "safeCallLua(): %1" ).arg( getLuaError() ) );
}

void CLuaRunner::onTimerTimeout( const QString &name )
{
	CScriptRunnerBase::onTimerTimeout( name );

	m_luaLib->lua_getglobal( L, "handle_timer_event" );
	if ( !m_luaLib->lua_isfunction( L, -1 ) ) {
		LOG( "handle_timer_event is NOT a function" );
		botInstance->quit();
		return;
	}

	m_luaLib->lua_pushstring( L, name.toUtf8().data() );
	if ( !safeCallLua( 1, 0 ) )
		LOG( fmt( "safeCallLua(): %1" ).arg( getLuaError() ) );
}

static int LUACB_getParticipants( CLuaLibLoader::lua_State *L )
{
	CLuaRunner *lr = LUARUN;
	LUALIB->lua_newtable( L );
	for ( int i = 0; i < lr->participants().size(); i++ ) {
		LUALIB->lua_pushinteger( L, i + 1 );
		LUALIB->lua_pushstring( L, lr->participants().at( i ).toUtf8().data() );
		LUALIB->lua_settable( L, -3 );
	}

	return 1;
}

static int LUACB_sendRoomMessage( CLuaLibLoader::lua_State *L )
{
	CLuaRunner *lr = LUARUN;
	size_t len = 0;
	const char *msg = LUALIB->lua_tolstring( L, 1, &len );
	bool ff = false;

	if ( LUALIB->lua_gettop( L ) > 1 )
		ff = ( bool )LUALIB->lua_toboolean( L, 2 );

	if ( msg )
		lr->sendMessage( lr->roomJid(), QString::fromUtf8( msg, len ), "", ff );

	return 0;
}

static int LUACB_log( CLuaLibLoader::lua_State *L )
{
	const char *msg = LUALIB->luaL_checkstring( L, 1 );
	if ( msg )
		OLOG(LUARUN, fmt( "%1" ).arg( msg ));

	return 0;
}

static int __internal_serialize( CLuaLibLoader::lua_State *L, int idx, char **pdest, size_t *pdsize, size_t *palloced )
{
	int     l_type  = LUALIB->lua_type( L, idx );
	char   *dst     = *pdest;
	size_t  size    = ( pdsize ? *pdsize : 0 );
	size_t  alloced = ( palloced ? *palloced : 0 );
	char    b1[128] = { 0 };
	char    b2[128] = { 0 };
	size_t  s1      = 0;
	size_t  s2      = 0;

#define REALLOC( required ) {                                                                     \
	size_t __remaining = alloced - size;                                                      \
	if ( __remaining < ( size_t )required ) {                                                 \
		size_t __count = alloced + required;                                              \
		__count += ( SERIALIZE_BUFF_INCREMENT - ( __count % SERIALIZE_BUFF_INCREMENT ) ); \
		char *__tmp = ( char * )realloc( dst, __count );                                  \
		if ( !__tmp ) {                                                                   \
			errno = ENOMEM;                                                           \
			return -1;                                                                \
		}                                                                                 \
		dst = __tmp;                                                                      \
		alloced = __count;                                                                \
		memset( dst + size, 0, __count - size );                                          \
	}                                                                                         \
}

	switch ( l_type ) {
		case LUA_TNIL: {
			REALLOC( 6 );
			strcpy( dst + size, "n:0:;" );
			size += 5;
			break;
		}
		case LUA_TBOOLEAN: {
			REALLOC( ( s1 = sprintf( b1, "b:1:%d;", LUALIB->lua_toboolean( L, idx ) ) ) + 1 );
			strcpy( dst + size, b1 );
			size += s1;
			break;
		}
		case LUA_TNUMBER: {
			CLuaLibLoader::lua_Number n = LUALIB->lua_tonumber( L, idx );
			CLuaLibLoader::lua_Number i = qFloor( n );
			CLuaLibLoader::lua_Number I = qCeil( n );
			char       t = 0;

			if ( i == n && n == I ) {
				s1 = sprintf( b1, "%ld", ( long )n );
				t  = 'i';
			} else {
				s1 = sprintf( b1, "%lf", n );
				t  = 'f';
			}
			s2 = sprintf( b2, "%d", ( int )s1 );

			REALLOC( s1 + s2 + 5 );
			sprintf( dst + size, "%c:%s:%s;", t, b2, b1 );
			size += s1 + s2 + 4;

			break;
		}
		case LUA_TSTRING: {
			const char *str = LUALIB->lua_tolstring( L, idx, &s2 );

			s1 = sprintf( b1, "%d", ( int )s2 );

			REALLOC( s1 + s2 + 5 );
			dst[size] = 's';
			dst[size + 1] = ':';
			strcpy( dst + size + 2, b1 );
			dst[size + s1 + 2] = ':';
			strcpy( dst + size + s1 + 3, str );
			dst[size + s1 + s2 + 3] = ';';
			dst[size + s1 + s2 + 4] = 0;

			size += s1 + s2 + 4;

			break;
		}
		case LUA_TTABLE: {
			size_t old_size = size;
			REALLOC( 17 );
			sprintf( dst + size, "a:%012lld:", ( unsigned long long )123456789012LL );
			size += 15;
			LUALIB->lua_pushnil( L );
			while ( LUALIB->lua_next( L, idx ) != 0 ) {
				__internal_serialize( L, LUALIB->lua_gettop( L ) - 1, &dst, &size, &alloced );
				__internal_serialize( L, LUALIB->lua_gettop( L ), &dst, &size, &alloced );
				LUALIB->lua_pop( L, 1 );
			}
			dst[size] = ';';
			s1 = sprintf( dst + old_size, "a:%d:", ( int )( size - old_size - 15 ) );

			size += 1;
			old_size += s1;

			memmove( dst + old_size, dst + old_size + 15 - s1, size - old_size + 1 );
			size -= ( 15 - s1 );

			break;
		}
	}

	if ( pdest    ) *pdest    = dst;
	if ( pdsize   ) *pdsize   = size;
	if ( palloced ) *palloced = alloced;

	return 0;
}

static int LUACB_serialize( CLuaLibLoader::lua_State *L )
{
	char   *dst = NULL;
	size_t  sz  = 0;

	if ( LUALIB->lua_gettop( L ) != 1 )
		LUALIB->luaL_error( L, "serialize() takes exactly 1 parameter" );

	if ( __internal_serialize( L, LUALIB->lua_gettop( L ), &dst, &sz, NULL ) < 0 ) {
		LUALIB->lua_pushnil( L );
		return 1;
	}

	LUALIB->lua_pushlstring( L, dst, sz );

	free( dst );

	return 1;
}

static inline bool __internal_unserialize_nil( CLuaLibLoader::lua_State *L, size_t, const char * )
{
	LUALIB->lua_pushnil( L );
	return true;
}

static inline bool __internal_unserialize_bool( CLuaLibLoader::lua_State *L, size_t size, const char *start )
{
	if ( size != 1 ) return false;
	if ( start[0] != '0' && start[0] != '1' ) return false;
	LUALIB->lua_pushboolean( L, start[0] - '0' );
	return true;
}

static inline bool __internal_unserialize_number( CLuaLibLoader::lua_State *L, size_t size, const char *start )
{
	bool ok = false;
	CLuaLibLoader::lua_Number n = QString( QByteArray( start, size ) ).toDouble( &ok );
	if ( !ok ) return false;
	LUALIB->lua_pushnumber( L, n );
	return true;
}

static inline bool __internal_unserialize_string( CLuaLibLoader::lua_State *L, size_t size, const char *start )
{
	LUALIB->lua_pushlstring( L, start, size );
	return true;
}

static inline bool __parser_type_and_length( const char *start, char *type, size_t *plen, size_t *ctl_len )
{
	size_t i = 0;
	bool ok = false;

	char op = start[i++];
	if ( !start[i] ) return false;
	if ( start[i++] != ':' ) return false;
	if ( !start[i] ) return false;

	while ( start[i] && isdigit( start[i] ) ) i++;
	if ( !start[i] ) return false;
	if ( start[i] != ':' ) return false;
	ptrdiff_t len = QString( QByteArray( start + 2, i - 2 ) ).toUInt( &ok );
	if ( !ok ) return false;

	i++;
	if ( !start[i] ) return false;

	*type = op;
	*plen = len;
	*ctl_len = i;

	return true;
}

static inline bool __internal_unserialize_table( CLuaLibLoader::lua_State *L, size_t size, const char *start )
{
	size_t i = 0;
	size_t c = 0;
	size_t l = 0;
	char   o = 0;
	int    t = LUALIB->lua_gettop( L );

#define RETURN_FALSE { if ( LUALIB->lua_gettop( L ) > t ) LUALIB->lua_pop( L, LUALIB->lua_gettop( L ) - t ); return false; }

	LUALIB->lua_newtable( L );
	while ( i < size ) {
		if ( !__parser_type_and_length( start + i, &o, &l, &c ) ) RETURN_FALSE;
		i += c;
		if ( i >= size ) RETURN_FALSE;
		if ( i + l >= size ) RETURN_FALSE;
		if ( start[i + l] != ';' ) RETURN_FALSE;
		switch ( o ) {
			case 'n': {
				RETURN_FALSE; /* nil can't be a key in a table */
				break;
			}
			case 'b': {
				if ( !__internal_unserialize_bool( L, l, start + i ) ) RETURN_FALSE;
				break;
			}
			case 'i':
			case 'f': {
				if ( !__internal_unserialize_number( L, l, start + i ) ) RETURN_FALSE;
				break;
			}
			case 's': {
				if ( !__internal_unserialize_string( L, l, start + i ) ) RETURN_FALSE;
				break;
			}
			case 'a': {
				if ( !__internal_unserialize_table( L, l, start + i ) ) RETURN_FALSE;
				break;
			}
			default: {
				RETURN_FALSE;
			}
		}

		i += l + 1;
		if ( i >= size ) RETURN_FALSE;
		if ( !__parser_type_and_length( start + i, &o, &l, &c ) ) RETURN_FALSE;
		i += c;
		if ( i >= size ) RETURN_FALSE;
		if ( i + l >= size ) RETURN_FALSE;
		if ( start[i + l] != ';' ) RETURN_FALSE;
		switch ( o ) {
			case 'n': {
				if ( !__internal_unserialize_nil( L, l, start + i ) ) RETURN_FALSE;
				break;
			}
			case 'b': {
				if ( !__internal_unserialize_bool( L, l, start + i ) ) RETURN_FALSE;
				break;
			}
			case 'i':
			case 'f': {
				if ( !__internal_unserialize_number( L, l, start + i ) ) RETURN_FALSE;
				break;
			}
			case 's': {
				if ( !__internal_unserialize_string( L, l, start + i ) ) RETURN_FALSE;
				break;
			}
			case 'a': {
				if ( !__internal_unserialize_table( L, l, start + i ) ) RETURN_FALSE;
				break;
			}
			default: {
				RETURN_FALSE;
			}
		}

		LUALIB->lua_settable( L, -3 );
		i += l + 1;
	}

	if ( i != size ) RETURN_FALSE;

	return true;
}

static int LUACB_unserialize( CLuaLibLoader::lua_State *L )
{
	size_t c = 0;
	size_t l = 0;
	char   o = 0;
	int    t = LUALIB->lua_gettop( L );

	if ( t != 1 || ( t == 1 && !LUALIB->lua_isstring( L, 1 ) ) )
		LUALIB->luaL_error( L, "unserialize() takes exactly 1 string as parameter" );

	const char *start = LUALIB->lua_tostring( L, 1 );
	size_t      size = strlen( start );

#define FAIL { if ( LUALIB->lua_gettop( L ) > t ) LUALIB->lua_pop( L, LUALIB->lua_gettop( L ) - t ); LUALIB->lua_pushboolean( L, 0 ); LUALIB->lua_pushnil( L ); return 2; }

	LUALIB->lua_pushboolean( L, 1 );

	if ( !__parser_type_and_length( start, &o, &l, &c ) ) FAIL;
	if ( c + l >= size ) FAIL;
	if ( start[c + l] != ';' ) FAIL;
	switch ( o ) {
		case 'n': {
			if ( !__internal_unserialize_nil( L, l, start + c ) ) FAIL;
			break;
		}
		case 'b': {
			if ( !__internal_unserialize_bool( L, l, start + c ) ) FAIL;
			break;
		}
		case 'i':
		case 'f': {
			if ( !__internal_unserialize_number( L, l, start + c ) ) FAIL;
			break;
		}
		case 's': {
			if ( !__internal_unserialize_string( L, l, start + c ) ) FAIL;
			break;
		}
		case 'a': {
			if ( !__internal_unserialize_table( L, l, start + c ) ) FAIL;
			break;
		}
		default: {
			FAIL;
		}
	}

	return 2;
}

static int LUACB_network_request( CLuaLibLoader::lua_State *L )
{
	CLuaRunner *lr = LUARUN;

	if ( LUALIB->lua_gettop( L ) == 1 && LUALIB->lua_isstring( L, 1 ) ) {
		lr->networkRequest( QString(), LUALIB->lua_tostring( L, 1 ) );
	} else if ( LUALIB->lua_gettop( L ) == 2 && LUALIB->lua_isstring( L, 1 ) && LUALIB->lua_isstring( L, 2 ) ) {
		lr->networkRequest( LUALIB->lua_tostring( L, 1 ), LUALIB->lua_tostring( L, 2 ) );
	} else {
		LUALIB->lua_pushinteger( L, -1 );
	}

	return 1;
}

static int LUACB_settimer( CLuaLibLoader::lua_State *L )
{
	CLuaRunner *lr = LUARUN;

	if ( LUALIB->lua_gettop( L ) == 1 && LUALIB->lua_isnumber( L, 1 ) ) {
		lr->createTimer( QString(), LUALIB->lua_tointeger( L, 1 ) );
	} else if ( LUALIB->lua_gettop( L ) == 2 && LUALIB->lua_isstring( L, 1 ) && LUALIB->lua_isnumber( L, 2 ) ) {
		lr->createTimer( LUALIB->lua_tostring( L, 1 ), LUALIB->lua_tointeger( L, 2 ) );
	}

	return 0;
}

static int LUACB_quit( CLuaLibLoader::lua_State * )
{
	botInstance->quit();
	return 0;
}

static int LUACB_tokenize( CLuaLibLoader::lua_State *L )
{
	QStringList lst;

	if ( LUALIB->lua_gettop( L ) == 1 && LUALIB->lua_isstring( L, 1 ) ) {
		QString str( LUALIB->lua_tostring( L, 1 ) );
		Utils::str_break( str, lst );
	}

	LUALIB->lua_newtable( L );
	for ( int i = 0; i < lst.size(); i++ ) {
		LUALIB->lua_pushinteger( L, i + 1 );
		LUALIB->lua_pushstring( L, lst.at( i ).toUtf8().data() );
		LUALIB->lua_settable( L, -3 );
	}

	return 1;
}

bool CLuaRunner::setupScript()
{
	int r = 0;
	QString fileName( fmt( "%1/%2" ).arg( botInstance->scriptPath() ).arg( m_script ) );

	if (!m_luaLib->isValid()) {
		LOG(fmt("%1").arg(m_luaLib->error()));
		return false;
	}

	foreach (const QString &vName, m_scriptEnv.keys()) {
		setenv(vName.toUtf8().data(), m_scriptEnv.value(vName).toUtf8().data(), 1);
	}

	L = m_luaLib->luaL_newstate();
	m_luaLib->luaL_openlibs( L );

	if ( ( r = m_luaLib->luaL_loadfile( L, fileName.toUtf8().data() ) ) != 0 ) {
		LOG( fmt( "m_luaLib->luaL_loadfile( '%1' ): ( %2 ) %3" ).arg( fileName ).arg( r ).arg( getLuaError() ) );
		return false;
	}

#define PRESENCE( __i ) m_luaLib->lua_pushinteger( L, ( int )QXmppPresence::__i ); m_luaLib->lua_setglobal( L, "PRES_" #__i )
	PRESENCE( Error );
	PRESENCE( Available );
	PRESENCE( Unavailable );
	PRESENCE( Subscribe );
	PRESENCE( Subscribed );
	PRESENCE( Unsubscribe );
	PRESENCE( Unsubscribed );
	PRESENCE( Probe );

#define STATUS( __i ) m_luaLib->lua_pushinteger( L, ( int )QXmppPresence::__i ); m_luaLib->lua_setglobal( L, "STAT_" #__i )
	STATUS( Online );
	STATUS( Away );
	STATUS( XA );
	STATUS( DND );
	STATUS( Chat );
	STATUS( Invisible );

	m_luaLib->lua_register( L, "quit", LUACB_quit );
	m_luaLib->lua_register( L, "serialize", LUACB_serialize );
	m_luaLib->lua_register( L, "unserialize", LUACB_unserialize );
	m_luaLib->lua_register( L, "network_request", LUACB_network_request );
	m_luaLib->lua_register( L, "get_participants", LUACB_getParticipants );
	m_luaLib->lua_register( L, "send_room_message", LUACB_sendRoomMessage );
	m_luaLib->lua_register( L, "dout", LUACB_log );
	m_luaLib->lua_register( L, "set_timer", LUACB_settimer );
	m_luaLib->lua_register( L, "tokenize", LUACB_tokenize );

	if ( !safeCallLua( 0, 0 ) ) {
		LOG( fmt( "Failed to run '%1': %2" ).arg( m_script ).arg( getLuaError() ) );
		return false;
	}

	m_luaLib->lua_getglobal( L, "handle_room_config" );
	m_luaLib->lua_getglobal( L, "handle_room_message" );
	m_luaLib->lua_getglobal( L, "handle_room_presence" );
	if ( !m_luaLib->lua_isfunction( L, -1 ) || !m_luaLib->lua_isfunction( L, -2 ) || !m_luaLib->lua_isfunction( L, -3 ) ) {
		LOG( "Script does not contain all needed callbacks" );
		return false;
	}

	return true;
}
