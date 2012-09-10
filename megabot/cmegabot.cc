#include "cxmppserver.h"
#include "cxmpproom.h"
#include "cmegabot.h"
#include "cscriptrunner.h"
#include "cscriptcontroller.h"

#include <QVariant>
#include <json.h>

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

void CMegaBot::log( int line, const QString &file, const QString &message )
{
	QString msg = QString( "[ %1 ][ %2:%3 ] %4" ).arg( getpid(), 5 ).arg( file, 21 ).arg( line, 4 ).arg( message );
	if ( m_forked ) {
		msg += "\n";
		FILE *f = fopen( m_logName.toUtf8().data(), "a+" );
		if ( f ) {
			fprintf( f, "%s", msg.toUtf8().data() );
			fclose( f );
		}
	} else {
		if ( m_mode == Master )
			printf( "\x1b[40m\x1b[36m%s\x1b[0m\n", msg.toUtf8().data() );
		else
			printf( "\x1b[40m\x1b[31m%s\x1b[0m\n", msg.toUtf8().data() );
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

void CMegaBot::qxmppLogEvent( const QXmppLogger::MessageType &type, const QString &msg )
{
	QString st = "";
	switch ( type ) {
		case QXmppLogger::DebugMessage: {
			st = "DBG";
			break;
		}
		case QXmppLogger::InformationMessage: {
			st = "INF";
			break;
		}
		case QXmppLogger::WarningMessage: {
			st = "WRN";
			break;
		}
		default: {
			break;
		}
	}

	LOG( fmt( "[QXmpp - %1] %2").arg( st ).arg( msg ) );
}

void CMegaBot::newKillSwitchConnection()
{
	QLocalSocket *conn = NULL;

	while ( ( conn = m_killSwitch->nextPendingConnection() ) != NULL ) {
		conn->setParent( this );
		connect( conn, SIGNAL( readyRead() ), this, SLOT( dataOnKillSwitchConnection() ) );
		LOG( "New kill-switch connection" );
	}
}

bool CMegaBot::initMaster( bool dofork )
{
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

	QFile config( "config.json" );
	bool ok = false;

	if ( !config.open( QIODevice::ReadOnly ) ) {
		LOG( fmt( "Failed to open config file: %1" ).arg( config.errorString() ) );
		return false;
	}
	QVariantMap result = Json::parse( config.readAll(), ok ).toMap();
	if ( !ok ) {
		LOG( fmt( "Failed to parse config file" ) );
		return false;
	}

	m_mode = Master;

	m_basePath = result["basePath"].toString();
	if ( m_basePath.isEmpty() ) {
		LOG( "Missing 'basePath' configuration" );
		return false;
	}

	m_logName = fmt( "%1/var/log/mblog.log" ).arg( m_basePath );

	foreach( const QVariant &vSrvName, result["servers"].toMap().keys() ) {
		QString server_handle = vSrvName.toString();
		if ( server_handle.isEmpty() ) continue;

		QVariantMap mSrv = result["servers"].toMap().value( server_handle ).toMap();

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
		connect( this, SIGNAL( configLoaded() ), server, SLOT( connectToServer() ) );
	}

	if ( !m_servers.size() ) {
		LOG( "No valid server configuration found" );
		return false;
	}

	emit configLoaded();

	return true;
}

bool CMegaBot::initScriptRunner( const QString &basePath, const QString &server, const QString &room, const QString &nickname, const QString &script, int fd )
{
	m_mode = ScriptRunner;

	m_basePath = basePath;

	QString fileName = fmt( "%1/share/scripts/%2" ).arg( m_basePath ).arg( script );
	LOG( fmt( "Running script '%1'" ).arg( fileName ) );
	m_logName = fmt( "%1/var/log/%2_%3_%4 - %5.log" ).arg( m_basePath ).arg( server ).arg( room ).arg( nickname ).arg( script );
	LOG( fmt( "Script log file: '%1'" ).arg( m_logName ) );

	m_forked = true;
	if ( ( m_runner = createRunner( fileName, fd ) ) == NULL )
		return false;

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
