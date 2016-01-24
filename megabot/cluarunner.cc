#include "cscriptrunner.h"
#include "cluarunner.h"
#include "ctlpackets.h"

#include <QXmppPresence.h>

#include <qmath.h>
#include <ctype.h>

CLuaRunner::CLuaRunner( const QString &handle, const QString &name, int fd, QObject *parent ) : CScriptRunnerBase( handle, name, fd, parent )
{
	L = NULL;
}

CLuaRunner::~CLuaRunner()
{
	if ( L ) {
		lua_getglobal( L, "handle_shutdown" );
		if ( lua_isfunction( L, -1 ) ) {
			if ( !safeCallLua( 0, 0 ) )
				LOG( fmt( "safeCallLua(): %1" ).arg( getLuaError() ) );
		}
		lua_close( L );
	}
}

bool CLuaRunner::safeCallLua( int nargs, int nresults )
{
	int st = lua_pcall( L, nargs, nresults, 0 );
	if ( st ) return false;
	return true;
}

QString CLuaRunner::getLuaError()
{
	return QString( lua_tostring( L, -1 ) );
}

void CLuaRunner::onRoomConfigPacket( const CRoomConfigPacket &pkt )
{
	CScriptRunnerBase::onRoomConfigPacket( pkt );

	lua_getglobal( L, "handle_room_config" );
	if ( !lua_isfunction( L, -1 ) ) {
		LOG( "handle_room_config() is NOT a function" );
		botInstance->quit();
		return;
	}
	lua_newtable( L );
	lua_pushstring( L, "roomName" );
	lua_pushstring( L, pkt.roomName().toUtf8().data() );
	lua_settable( L, -3 );
	lua_pushstring( L, "roomJid" );
	lua_pushstring( L, pkt.roomJid().toUtf8().data() );
	lua_settable( L, -3 );
	lua_pushstring( L, "nickName" );
	lua_pushstring( L, pkt.nickName().toUtf8().data() );
	lua_settable( L, -3 );
	lua_pushstring( L, "topic" );
	lua_pushstring( L, pkt.topic().toUtf8().data() );
	lua_settable( L, -3 );
	if ( !safeCallLua( 1, 0 ) )
		LOG( fmt( "safeCallLua(): %1" ).arg( getLuaError() ) );
}

void CLuaRunner::onRoomMessagePacket( const CRoomMessagePacket &pkt )
{
	CScriptRunnerBase::onRoomMessagePacket( pkt );

	if ( QXmppUtils::jidToResource( pkt.from() ) == m_nickName )
		return;

	lua_getglobal( L, "handle_room_message" );
	if ( !lua_isfunction( L, -1 ) ) {
		LOG( "handle_room_message() is NOT a function" );
		botInstance->quit();
		return;
	}
	lua_newtable( L );
	lua_pushstring( L, "from" );
	lua_pushstring( L, pkt.from().toUtf8().data() );
	lua_settable( L, -3 );
	lua_pushstring( L, "body" );
	lua_pushstring( L, pkt.body().toUtf8().data() );
	lua_settable( L, -3 );
	lua_pushstring( L, "nickName" );
	lua_pushstring( L, QXmppUtils::jidToResource( pkt.from() ).toUtf8().data() );
	lua_settable( L, -3 );
	lua_pushstring( L, "serverTime" );
	lua_pushnumber( L, pkt.serverTime() );
	lua_settable( L, -3 );
	lua_pushstring( L, "localTime" );
	lua_pushnumber( L, pkt.localTime() );
	lua_settable( L, -3 );
	if ( !safeCallLua( 1, 0 ) )
		LOG( fmt( "safeCallLua(): %1" ).arg( getLuaError() ) );
}

void CLuaRunner::onRoomPresencePacket( const CRoomPresencePacket &pkt )
{
	CScriptRunnerBase::onRoomPresencePacket( pkt );

	lua_getglobal( L, "handle_room_presence" );
	if ( !lua_isfunction( L, -1 ) ) {
		LOG( "handle_room_presence() is NOT a function" );
		botInstance->quit();
		return;
	}
	lua_newtable( L );
	lua_pushstring( L, "who" );
	lua_pushstring( L, pkt.who().toUtf8().data() );
	lua_settable( L, -3 );
	lua_pushstring( L, "nickName" );
	lua_pushstring( L, QXmppUtils::jidToResource( pkt.who() ).toUtf8().data() );
	lua_settable( L, -3 );
	lua_pushstring( L, "presence" );
	lua_pushinteger( L, ( int )pkt.presenceType() );
	lua_settable( L, -3 );
	lua_pushstring( L, "statusType" );
	lua_pushinteger( L, ( int )pkt.statusType() );
	lua_settable( L, -3 );
	lua_pushstring( L, "statusText" );
	lua_pushstring( L, pkt.statusText().toUtf8().data() );
	lua_settable( L, -3 );
	lua_pushstring( L, "statusPriority" );
	lua_pushinteger( L, pkt.statusPriority() );
	lua_settable( L, -3 );
	if ( !safeCallLua( 1, 0 ) )
		LOG( fmt( "safeCallLua(): %1" ).arg( getLuaError() ) );
}

void CLuaRunner::onNetworkRequestFinished( bool allOk, const QString &name, const QString &url, const QByteArray &data )
{
	CScriptRunnerBase::onNetworkRequestFinished( allOk, name, url, data );

	lua_getglobal( L, "handle_network_reply" );
	if ( !lua_isfunction( L, -1 ) ) {
		LOG( "handle_network_reply is NOT a function" );
		botInstance->quit();
		return;
	}

	lua_newtable( L );
	lua_pushstring( L, "status" );
	lua_pushboolean( L, ( int )allOk );
	lua_settable( L, -3 );
	lua_pushstring( L, "name" );
	lua_pushstring( L, name.toUtf8().data() );
	lua_settable( L, -3 );
	lua_pushstring( L, "data" );
	lua_pushlstring( L, data.data(), data.size() );
	lua_settable( L, -3 );
	lua_pushstring( L, "url" );
	lua_pushstring( L, url.toUtf8().data() );
	lua_settable( L, -3 );
	if ( !safeCallLua( 1, 0 ) )
		LOG( fmt( "safeCallLua(): %1" ).arg( getLuaError() ) );
}

void CLuaRunner::onTimerTimeout( const QString &name )
{
	CScriptRunnerBase::onTimerTimeout( name );

	lua_getglobal( L, "handle_timer_event" );
	if ( !lua_isfunction( L, -1 ) ) {
		LOG( "handle_timer_event is NOT a function" );
		botInstance->quit();
		return;
	}

	lua_pushstring( L, name.toUtf8().data() );
	if ( !safeCallLua( 1, 0 ) )
		LOG( fmt( "safeCallLua(): %1" ).arg( getLuaError() ) );
}

static int LUACB_getParticipants( lua_State *L )
{
	CLuaRunner *lr = dynamic_cast<CLuaRunner *>( global_runner );
	if ( !lr ) return 0;

	lua_newtable( L );
	for ( int i = 0; i < lr->participants().size(); i++ ) {
		lua_pushinteger( L, i + 1 );
		lua_pushstring( L, lr->participants().at( i ).toUtf8().data() );
		lua_settable( L, -3 );
	}

	return 1;
}

static int LUACB_sendRoomMessage( lua_State *L )
{
	CLuaRunner *lr = dynamic_cast<CLuaRunner *>( global_runner );
	if ( !lr ) return 0;

	size_t len = 0;
	const char *msg = lua_tolstring( L, 1, &len );
	bool ff = false;

	if ( lua_gettop( L ) > 1 )
		ff = ( bool )lua_toboolean( L, 2 );

	if ( msg )
		lr->sendMessage( lr->roomJid(), QString::fromUtf8( msg ), "", ff );

	return 0;
}

static int LUACB_log( lua_State *L )
{
	const char *msg = luaL_checkstring( L, 1 );
	if ( msg )
		OLOG( global_runner, fmt( "%1" ).arg( msg ) );

	return 0;
}

static int __internal_serialize( lua_State *L, int idx, char **pdest, size_t *pdsize, size_t *palloced )
{
	int     l_type  = lua_type( L, idx );
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
			REALLOC( ( s1 = sprintf( b1, "b:1:%d;", lua_toboolean( L, idx ) ) ) + 1 );
			strcpy( dst + size, b1 );
			size += s1;
			break;
		}
		case LUA_TNUMBER: {
			lua_Number n = lua_tonumber( L, idx );
			lua_Number i = qFloor( n );
			lua_Number I = qCeil( n );
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
			const char *str = lua_tolstring( L, idx, &s2 );

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
			lua_pushnil( L );
			while ( lua_next( L, idx ) != 0 ) {
				__internal_serialize( L, lua_gettop( L ) - 1, &dst, &size, &alloced );
				__internal_serialize( L, lua_gettop( L ), &dst, &size, &alloced );
				lua_pop( L, 1 );
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

static int LUACB_serialize( lua_State *L )
{
	char   *dst = NULL;
	size_t  sz  = 0;

	if ( lua_gettop( L ) != 1 )
		luaL_error( L, "serialize() takes exactly 1 parameter" );

	if ( __internal_serialize( L, lua_gettop( L ), &dst, &sz, NULL ) < 0 ) {
		lua_pushnil( L );
		return 1;
	}

	lua_pushlstring( L, dst, sz );

	free( dst );

	return 1;
}

static inline bool __internal_unserialize_nil( lua_State *L, size_t, const char * )
{
	lua_pushnil( L );
	return true;
}

static inline bool __internal_unserialize_bool( lua_State *L, size_t size, const char *start )
{
	if ( size != 1 ) return false;
	if ( start[0] != '0' && start[0] != '1' ) return false;
	lua_pushboolean( L, start[0] - '0' );
	return true;
}

static inline bool __internal_unserialize_number( lua_State *L, size_t size, const char *start )
{
	bool ok = false;
	lua_Number n = QString( QByteArray( start, size ) ).toDouble( &ok );
	if ( !ok ) return false;
	lua_pushnumber( L, n );
	return true;
}

static inline bool __internal_unserialize_string( lua_State *L, size_t size, const char *start )
{
	lua_pushlstring( L, start, size );
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

static inline bool __internal_unserialize_table( lua_State *L, size_t size, const char *start )
{
	size_t i = 0;
	size_t c = 0;
	size_t l = 0;
	char   o = 0;
	int    t = lua_gettop( L );

#define RETURN_FALSE { if ( lua_gettop( L ) > t ) lua_pop( L, lua_gettop( L ) - t ); return false; }

	lua_newtable( L );
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

		lua_settable( L, -3 );
		i += l + 1;
	}

	if ( i != size ) RETURN_FALSE;

	return true;
}

static int LUACB_unserialize( lua_State *L )
{
	size_t c = 0;
	size_t l = 0;
	char   o = 0;
	int    t = lua_gettop( L );

	if ( t != 1 || ( t == 1 && !lua_isstring( L, 1 ) ) )
		luaL_error( L, "unserialize() takes exactly 1 string as parameter" );

	const char *start = lua_tostring( L, 1 );
	size_t      size = strlen( start );

#define FAIL { if ( lua_gettop( L ) > t ) lua_pop( L, lua_gettop( L ) - t ); lua_pushboolean( L, 0 ); lua_pushnil( L ); return 2; }

	lua_pushboolean( L, 1 );

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

static int LUACB_network_request( lua_State *L )
{
	CLuaRunner *lr = dynamic_cast<CLuaRunner *>( global_runner );
	if ( !lr ) return 0;

	if ( lua_gettop( L ) == 1 && lua_isstring( L, 1 ) ) {
		lr->networkRequest( QString(), lua_tostring( L, 1 ) );
	} else if ( lua_gettop( L ) == 2 && lua_isstring( L, 1 ) && lua_isstring( L, 2 ) ) {
		lr->networkRequest( lua_tostring( L, 1 ), lua_tostring( L, 2 ) );
	} else {
		lua_pushinteger( L, -1 );
	}

	return 1;
}

static int LUACB_settimer( lua_State *L )
{
	CLuaRunner *lr = dynamic_cast<CLuaRunner *>( global_runner );
	if ( !lr ) return 0;

	if ( lua_gettop( L ) == 1 && lua_isnumber( L, 1 ) ) {
		lr->createTimer( QString(), lua_tointeger( L, 1 ) );
	} else if ( lua_gettop( L ) == 2 && lua_isstring( L, 1 ) && lua_isnumber( L, 2 ) ) {
		lr->createTimer( lua_tostring( L, 1 ), lua_tointeger( L, 2 ) );
	}

	return 0;
}

static int LUACB_quit( lua_State * )
{
	botInstance->quit();
	return 0;
}

static int LUACB_tokenize( lua_State *L )
{
	QStringList lst;

	if ( lua_gettop( L ) == 1 && lua_isstring( L, 1 ) ) {
		QString str( lua_tostring( L, 1 ) );
		Utils::str_break( str, lst );
	}

	lua_newtable( L );
	for ( int i = 0; i < lst.size(); i++ ) {
		lua_pushinteger( L, i + 1 );
		lua_pushstring( L, lst.at( i ).toUtf8().data() );
		lua_settable( L, -3 );
	}

	return 1;
}

bool CLuaRunner::setupScript()
{
	int r = 0;

	QString fileName( fmt( "%1/%2" ).arg( botInstance->scriptPath() ).arg( m_script ) );
	QString moduleSearchPath( fmt( ";;%1/share/lib/lua/?.lua" ).arg( botInstance->basePath() ) );
	QString moduleSearchCPath( fmt( ";;%1/share/lib/lua/lib?.so" ).arg( botInstance->basePath() ) );
	setenv( "LUA_PATH", moduleSearchPath.toUtf8().data(), 1 );
	setenv( "LUA_CPATH", moduleSearchCPath.toUtf8().data(), 1 );

	L = luaL_newstate();
	luaL_openlibs( L );

	if ( ( r = luaL_loadfile( L, fileName.toUtf8().data() ) ) != 0 ) {
		LOG( fmt( "luaL_loadfile( '%1' ): ( %2 ) %3" ).arg( fileName ).arg( r ).arg( getLuaError() ) );
		return false;
	}

#define PRESENCE( __i ) lua_pushinteger( L, ( int )QXmppPresence::__i ); lua_setglobal( L, "PRES_" #__i )
	PRESENCE( Error );
	PRESENCE( Available );
	PRESENCE( Unavailable );
	PRESENCE( Subscribe );
	PRESENCE( Subscribed );
	PRESENCE( Unsubscribe );
	PRESENCE( Unsubscribed );
	PRESENCE( Probe );

#define STATUS( __i ) lua_pushinteger( L, ( int )QXmppPresence::__i ); lua_setglobal( L, "STAT_" #__i )
	STATUS( Online );
	STATUS( Away );
	STATUS( XA );
	STATUS( DND );
	STATUS( Chat );
	STATUS( Invisible );

	lua_register( L, "quit", LUACB_quit );
	lua_register( L, "serialize", LUACB_serialize );
	lua_register( L, "unserialize", LUACB_unserialize );
	lua_register( L, "network_request", LUACB_network_request );
	lua_register( L, "get_participants", LUACB_getParticipants );
	lua_register( L, "send_room_message", LUACB_sendRoomMessage );
	lua_register( L, "dout", LUACB_log );
	lua_register( L, "set_timer", LUACB_settimer );
	lua_register( L, "tokenize", LUACB_tokenize );

	if ( !safeCallLua( 0, 0 ) ) {
		LOG( fmt( "Failed to run '%1': %2" ).arg( m_script ).arg( getLuaError() ) );
		return false;
	}

	lua_getglobal( L, "handle_room_config" );
	lua_getglobal( L, "handle_room_message" );
	lua_getglobal( L, "handle_room_presence" );
	if ( !lua_isfunction( L, -1 ) || !lua_isfunction( L, -2 ) || !lua_isfunction( L, -3 ) ) {
		LOG( "Script does not contain all needed callbacks" );
		return false;
	}

	return true;
}
