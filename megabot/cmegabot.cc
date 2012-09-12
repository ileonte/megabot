#include "cxmppserver.h"
#include "cxmpproom.h"
#include "cmegabot.h"
#include "cscriptrunner.h"
#include "cscriptcontroller.h"

using namespace QtJson;

void CMegaBot::writeDummyConfig( const QString &path PNU )
{
}

CMegaBot::CMegaBot( int &argc, char **argv ) : QCoreApplication( argc, argv )
{
	m_mode   = Unknown;
	m_forked = false;
	m_runner = NULL;
}

CMegaBot::~CMegaBot( void )
{
	if ( m_mode == Master ) {
		QSettings setts;

		setts.beginWriteArray( "Servers" );
		for ( int i = 0; i < m_servers.size(); i++ ) {
			CXMPPServer *server = m_servers[i];

			setts.setArrayIndex( i );
			setts.setValue( "host",           server->host() );
			setts.setValue( "domain",         server->domain() );
			setts.setValue( "account",        server->account() );
			setts.setValue( "resource",       server->resource() );
			setts.setValue( "password",       server->password() );
			setts.setValue( "conferenceHost", server->conferenceHost() );

			setts.remove( "rooms" );
			setts.beginWriteArray( "rooms" );
			int k = 0;
			for ( int j = 0; j < server->m_rooms.size(); j++ ) {
				CXMPPRoom *room = server->m_rooms[j];
				if ( !room->autoJoin() ) continue;

				setts.setArrayIndex( k++ );
				setts.setValue( "roomName", room->roomName() );
				setts.setValue( "nickName", room->nickName() );
				setts.setValue( "password", room->password() );

				int m = 0;
				setts.beginWriteArray( "scripts" );
				for ( int n = 0; n < room->m_scripts.size(); n++ ) {
					CScriptController *script = room->m_scripts[n];
					if ( !script->autoRun() ) continue;

					setts.setArrayIndex( m++ );
					setts.setValue( "script", script->script() );
				}
				setts.endArray();
			}
			setts.endArray();
		}
		setts.endArray();

		for ( int i = 0; i < m_servers.size(); i++ )
			delete m_servers[i];
		m_servers.clear();
	} else {
		delete m_runner;
	}
}

void CMegaBot::triggerKillSwitch()
{
	QLocalSocket ks;

	ks.connectToServer( MB_KILL_SWITCH );
	if ( !ks.waitForConnected( -1 ) ) {
		LOG( fmt( "Failed to trigger kill-switch: %1" ).arg( ks.errorString() ) );
		return;
	}

	ks.write( "quit" );
	ks.waitForBytesWritten( -1 );

	LOG( "Kill-switch triggered" );
	ks.disconnectFromServer();
}

void CMegaBot::dataOnKillSwitchConnection()
{
	LOG( "Local kill-switch activated" );
	quit();
}

void CMegaBot::newKillSwitchConnection()
{
	QLocalSocket *conn = NULL;

	while ( ( conn = m_killSwitch->nextPendingConnection() ) != NULL ) {
		conn->setParent( this );
		connect( conn, SIGNAL( readyRead() ), this, SLOT( dataOnKillSwitchConnection() ) );
	}
}

bool CMegaBot::loadConfig()
{
	QFileInfo fi[] = { QString( "/etc/MegaBot/config.json" ), m_basePath + "/etc/config.json" };
	bool ok = false;

	for ( unsigned i = 0; i < sizeof( fi ) / sizeof( fi[0] ); i++ ) {
		QFile config( fi[i].absoluteFilePath() );

		if ( !config.open( QIODevice::ReadOnly ) ) {
			LOG( fmt( "Failed to open '%1': %2" ).arg( fi[i].absoluteFilePath() ).arg( config.errorString() ) );
			continue;
		}
		m_config = Json::parse( config.readAll(), ok ).toMap();
		if ( !ok ) {
			LOG( fmt( "Failed to parse file '%1'" ).arg( fi[i].absoluteFilePath() ) );
			continue;
		}

		m_configPath = fi[i].absoluteFilePath();
		return true;
	}

	return false;
}

bool CMegaBot::initMaster(bool dofork, const QString &basePath )
{
	m_mode = Master;

	if ( !basePath.isEmpty() ) m_basePath = basePath;
	else m_basePath = "/opt/MegaBot";

	m_scriptPath = fmt( "%1/share/scripts" ).arg( m_basePath );
	m_logPath = fmt( "%1/var/log" ).arg( m_basePath );

	if ( !loadConfig() )
		return false;

	m_killSwitch = new QLocalServer( this );
	if ( !m_killSwitch->listen( MB_KILL_SWITCH ) ) {
		LOG( fmt( "Failed to create MegaBot kill switch: %1" ).arg( m_killSwitch->errorString() ) );
		delete m_killSwitch;
		return false;
	}
	connect( m_killSwitch, SIGNAL( newConnection() ), this, SLOT( newKillSwitchConnection() ) );

	if ( dofork ) {
		if ( daemon( 1, 0 ) < 0 ) {
			LOG( fmt( "Failed to daemonize: %1" ).arg( strerror( errno ) ) );
			return false;
		}
		m_forked = true;
	}

	foreach( const QVariant &vSrvName, m_config["servers"].toMap().keys() ) {
		QString server_handle = vSrvName.toString();
		if ( server_handle.isEmpty() ) continue;

		QVariantMap mSrv = m_config["servers"].toMap().value( server_handle ).toMap();

		CXMPPServer *server = new CXMPPServer( server_handle, this );
		server->setHost( mSrv["host"].toString() );
		server->setDomain( mSrv["domain"].toString() );
		server->setAccount( mSrv["account"].toString() );
		server->setResource( mSrv["resource"].toString() );
		server->setPassword( mSrv["password"].toString() );
		server->setConferenceHost( mSrv["conferenceHost"].toString() );
		if ( server->isEmpty() ) {
			delete server;
			continue;
		}

		foreach ( const QString &roomName, mSrv["rooms"].toMap().keys() ) {
			QVariantMap mRoom = mSrv["rooms"].toMap().value( roomName ).toMap();

			CXMPPRoom *room = new CXMPPRoom( server );
			room->setRoomName( roomName );
			room->setNickName( mRoom["nickName"].toString() );
			room->setPassword( mRoom["password"].toString() );
			room->setAutoJoin( true );
			if ( room->isEmpty() ) {
				delete room;
				continue;
			}

			foreach ( const QVariant &vScript, mRoom["scripts"].toList() ) {
				QString script_name = vScript.toString();
				if ( script_name.isEmpty() ) continue;
				CScriptController *script = room->findScript( script_name );
				if ( !script ) {
					script = new CScriptController( room, script_name );
					script->setAutoRun( true );
					room->addScript( script );
				}
			}

			server->addRoom( room );
		}

		m_servers.append( server );
		connect( this, SIGNAL( botInitialized() ), server, SLOT( connectToServer() ) );
	}

	if ( !m_servers.size() ) {
		LOG( "No valid server configuration found" );
		return false;
	}

	emit botInitialized();

	return true;
}

bool CMegaBot::initScriptRunner()
{
	QMap<QString, QString> varMap;
	QString error;
	QString script;
	QString server;
	QString handle;
	QString room;
	QString nickname;

	m_mode = ScriptRunner;
	m_forked = true;

	varMap["MEGABOT_CONTROL_SOCKET"] = "";
	varMap["MEGABOT_SERVER"]         = "";
	varMap["MEGABOT_HANDLE"]         = "";
	varMap["MEGABOT_ROOM"]           = "";
	varMap["MEGABOT_NICKNAME"]       = "";
	varMap["MEGABOT_BASEPATH"]       = "";
	varMap["MEGABOT_SCRIPT"]         = "";

	foreach ( const QString &var, varMap.keys() ) {
		varMap[var] = CMegaBot::getEnv( var );
		if ( varMap[var].isEmpty() ) error += " " + var;
	}
	if ( !error.isEmpty() ) return false;

	bool ok = true;
	int fd = varMap["MEGABOT_CONTROL_SOCKET"].toInt( &ok );
	if ( !ok || ( ok && fd < 0 ) ) return false;

	m_basePath   = varMap["MEGABOT_BASEPATH"];
	m_scriptPath = fmt( "%1/share/scripts" ).arg( m_basePath );
	m_logPath    = fmt( "%1/var/log" ).arg( m_basePath );

	script       = varMap["MEGABOT_SCRIPT"];
	server       = varMap["MEGABOT_SERVER"];
	handle       = varMap["MEGABOT_HANDLE"];
	room         = varMap["MEGABOT_ROOM"];
	nickname     = varMap["MEGABOT_NICKNAME"];

	if ( ( m_runner = createRunner( handle, script, fd ) ) == NULL ) return false;
	m_runner->setInitialConfig( server, room, nickname );

	return true;
}

void CMegaBot::quit()
{
	if ( m_mode == Master ) {
		for ( int i = 0; i < m_servers.size(); i++ )
			m_servers[i]->disconnectFromServer();
	}
	QCoreApplication::quit();
}

void CMegaBot::closeAllSockets( int except )
{
	for ( int i = 0; i < m_servers.size(); i++ ) {
		CXMPPServer *server = m_servers[i];
		for ( int j = 0; j < server->m_rooms.size(); j++ ) {
			CXMPPRoom *room = server->m_rooms[j];
			for ( int k = 0; k < room->m_scripts.size(); k++ ) {
				CScriptController *script = room->m_scripts[k];
				if ( script->m_sockfds[0] != except ) close( script->m_sockfds[0] );
				if ( script->m_sockfds[1] != except ) close( script->m_sockfds[1] );
			}
		}
	}
}
