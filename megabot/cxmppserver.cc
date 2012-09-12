#include "cxmppserver.h"
#include "cxmpproom.h"
#include "utils.h"
#include "cscriptcontroller.h"

#include <QXmppUtils.h>
#include <QXmppMessage.h>
#include <QXmppVersionManager.h>
#include <QXmppGlobal.h>

CXMPPServer::CXMPPServer( const QString &handle, QObject *parent ) : QObject( parent )
{
	m_handle = handle;
	m_client = new QXmppClient;
	m_mucman = new QXmppMucManager;
	m_userDisconnect = false;

	m_client->versionManager().setClientName( "MegaBot/QXmpp" );
	m_client->versionManager().setClientVersion( "0.4.0/" + QXmppVersion() );
	m_client->versionManager().setClientOs( "Linux" );

	m_client->addExtension( m_mucman );
}

CXMPPServer::~CXMPPServer()
{
	if ( m_client && m_client->isConnected() ) {
		LOG( fmt( "Disconnecting from '%1'" ).arg( jid() ) );
		disconnectFromServer();
		delete m_client;
	}

	for ( int i = 0; i < m_rooms.size(); i++ )
		delete m_rooms[i];
	m_rooms.clear();
}

bool CXMPPServer::setJid( const QString &jid )
{
	QString acc  = QXmppUtils::jidToUser( jid );
	QString host = QXmppUtils::jidToDomain( jid );
	QString res  = QXmppUtils::jidToResource( jid );

	if ( !acc.isEmpty() && !host.isEmpty() && !res.isEmpty() ) {
		m_host     = host;
		m_account  = acc;
		m_resource = res;

		return true;
	}

	return false;
}

void CXMPPServer::connectToServer()
{
	QXmppConfiguration cfg;
	cfg.setHost( m_host );
	cfg.setDomain( m_domain );
	cfg.setUser( m_account );
	cfg.setResource( m_resource );
	cfg.setPassword( m_password );
	cfg.setIgnoreSslErrors( true );
	cfg.setAutoReconnectionEnabled( true );
	cfg.setStreamSecurityMode( QXmppConfiguration::TLSRequired );

	QXmppPresence pres( QXmppPresence::Available );
	pres.setAvailableStatusType( QXmppPresence::Online );
	pres.setStatusText( "Doctors are rich ? When did this happen !?" );

	connect( m_client, SIGNAL( connected() ), this, SLOT( clientConnected() ) );
	connect( m_client, SIGNAL( disconnected() ), this, SLOT( clientDisconnected() ) );
	connect( m_client, SIGNAL( messageReceived( QXmppMessage ) ), this, SLOT( messageReceived( QXmppMessage ) ) );
	connect( m_client, SIGNAL( presenceReceived( QXmppPresence ) ), this, SLOT( presenceReceived( QXmppPresence ) ) );

	QXmppLogger *logger = new QXmppLogger( m_client );
	logger->setLoggingType( QXmppLogger::SignalLogging );
	logger->setMessageTypes( QXmppLogger::DebugMessage | QXmppLogger::InformationMessage | QXmppLogger::WarningMessage );
	connect( logger, SIGNAL( message( QXmppLogger::MessageType, QString ) ), this, SLOT( qxmppLogEvent( QXmppLogger::MessageType, QString ) ) );
	m_client->setLogger( logger );

	m_client->connectToServer( cfg, pres );
}

void CXMPPServer::disconnectFromServer()
{
	if ( m_client->isConnected() ) {
		m_userDisconnect = true;
		for ( int i = 0; i < m_rooms.size(); i++ ) {
			LOG( fmt( "Leaving room '%1' on server '%2'" ).arg( m_rooms[i]->roomName() ).arg( jid() ) );
			m_rooms[i]->part();
		}
		m_client->disconnectFromServer();
	}
}

void CXMPPServer::qxmppLogEvent( const QXmppLogger::MessageType &type, const QString &msg )
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

CXMPPRoom *CXMPPServer::findRoom( const QString &bareJid )
{
	for ( int i = 0; i < m_rooms.size(); i++ )
		if ( m_rooms[i]->bareJid() == bareJid ) return m_rooms[i];
	return NULL;
}

void CXMPPServer::clientConnected()
{
	LOG( fmt( "Connected to '%1' ( '%2' )" ).arg( jid() ).arg( m_handle ) );
	for ( int i = 0; i < m_rooms.size(); i++ ) {
		if ( m_rooms[i]->join() )
			LOG( fmt( "Joined room '%1'" ).arg( m_rooms[i]->bareJid() ) );
		else
			LOG( fmt( "Failed to join '%1'" ).arg( m_rooms[i]->bareJid() ) );
	}
}

void CXMPPServer::clientDisconnected()
{
	LOG( fmt( "Disconnected from '%1'" ).arg( jid() ) );
	for ( int i = 0; i < m_rooms.size(); i++ )
		m_rooms[i]->part();

	if ( !m_userDisconnect ) {
		LOG( "Scheduling reconnect event ( 10 seconds )" );
		m_client->deleteLater();
		m_client = new QXmppClient;
		m_client->addExtension( m_mucman );
		QTimer::singleShot( 10000, this, SLOT( connectToServer() ) );
	}
}

bool CXMPPServer::checkLoggedIn( const QString &jid ) const
{
	return m_loggedInUsers.contains( jid );
}

bool CXMPPServer::checkUsernameAndPassword( const QString &username, const QString &password )
{
	if ( botInstance->config()["servers"].toMap().value( m_handle ).toMap().value( "users" ).toMap().value( username ).toString() == password )
		return true;

	return botInstance->config()["users"].toMap().value( username ).toString() == password;
}

void CXMPPServer::handleBotCommand( const QXmppMessage &msg )
{
	QStringList args;
	Utils::str_break( msg.body(), args );
	if ( !args.size() ) return;

	/* COMMANDS */
	if ( args[0] == "login" ) {
		/* login <username> <password> */
		if ( args.size() != 3 ) {
			m_client->sendMessage( msg.from(), "Usage: login <username> <password>" );
			return;
		}

		if ( m_loggedInUsers[msg.from()] == args[1] ) {
			m_client->sendMessage( msg.from(), "You are already logged in as that user" );
			return;
		}

		if ( !checkUsernameAndPassword( args[1], args[2] ) ) {
			m_client->sendMessage( msg.from(), "Incorrect username and/or password" );
			return;
		}

		m_client->sendMessage( msg.from(), fmt( "JID '%1' now logged in as MegaBot user '%2'" ).arg( msg.from() ).arg( args[1] ) );
		m_loggedInUsers[msg.from()] = args[1];
		return;
	}

	if ( args[0] == "logout" ) {
		if ( checkLoggedIn( msg.from() ) ) {
			m_loggedInUsers.remove( msg.from() );
			m_client->sendMessage( msg.from(), "You are now logged out" );
		}
		return;
	}

	if ( args[0] == "enter" || args[0] == "join" ) {
		/* enter <nick> <room> [password] */
		if ( !checkLoggedIn( msg.from() ) ) {
			m_client->sendMessage( msg.from(), "You are not logged in" );
			return;
		}
		if ( args.size() < 3 || args.size() > 4 ) {
			m_client->sendMessage( msg.from(), fmt( "Usage: %1 <nick> <room> [password]" ).arg( args[0] ) );
			return;
		}

		QString    roomJid = args[2] + "@" + m_conferenceHost;
		CXMPPRoom *room    = findRoom( roomJid );
		if ( !room ) {
			room = new CXMPPRoom( this );
			room->setNickName( args[1] );
			room->setRoomName( args[2] );
			if ( args.size() > 3 ) room->setPassword( args[3] );
			m_rooms.append( room );
		}

		if ( !room->joined() ) {
			if ( room->join() ) {
				m_client->sendMessage( msg.from(), fmt( "Successfully entered room '%1'"  ).arg( roomJid ) );
				return;
			} else {
				m_client->sendMessage( msg.from(), fmt( "Failed to enter room '%1'"  ).arg( roomJid ) );
				m_rooms.removeAll( room );
				delete room;
				return;
			}
		} else {
			m_client->sendMessage( msg.from(), fmt( "I am already in room '%1'" ).arg( roomJid ) );
			return;
		}

		return;
	}

	if ( args[0] == "leave" || args[0] == "part" ) {
		/* leave <room> */
		if ( !checkLoggedIn( msg.from() ) ) {
			m_client->sendMessage( msg.from(), "You are not logged in" );
			return;
		}
		if ( args.size() != 2 ) {
			m_client->sendMessage( msg.from(), fmt( "Usage: %1 <room>" ).arg( args[0] ) );
			return;
		}

		QString    roomJid = args[1] + "@" + m_conferenceHost;
		CXMPPRoom *room    = findRoom( roomJid );
		if ( room ) {
			room->part();
			m_client->sendMessage( msg.from(), fmt( "Left room '%1'" ).arg( roomJid ) );
		} else {
			m_client->sendMessage( msg.from(), fmt( "Not in room '%1', nothing to leave" ).arg( roomJid ) );
		}

		return;
	}

	if ( args[0] == "autoenter" || args[0] == "autojoin" ) {
		/* autojoin <room> */
		if ( !checkLoggedIn( msg.from() ) ) {
			m_client->sendMessage( msg.from(), "You are not logged in" );
			return;
		}
		if ( args.size() != 2 ) {
			m_client->sendMessage( msg.from(), fmt( "Usage: %1 <room>" ).arg( args[0] ) );
			return;
		}

		QString    roomJid = args[1] + "@" + m_conferenceHost;
		CXMPPRoom *room    = findRoom( roomJid );
		if ( room ) {
			room->toggleAutoJoin();
			if ( room->autoJoin() )
				m_client->sendMessage( msg.from(), fmt( "The AUTOJOIN flag on room '%1' is now ON" ).arg( roomJid ) );
			else
				m_client->sendMessage( msg.from(), fmt( "The AUTOJOIN flag on room '%1' is now OFF" ).arg( roomJid ) );
		} else {
			m_client->sendMessage( msg.from(), "The AUTOJOIN flag can only be toggled on joined rooms" );
		}

		return;
	}

	if ( args[0] == "say" ) {
		if ( !checkLoggedIn( msg.from() ) ) {
			m_client->sendMessage( msg.from(), "You are not logged in" );
			return;
		}
		if ( args.size() < 3 ) {
			m_client->sendMessage( msg.from(), "Usage: say <room> ..." );
			return;
		}

		QString    roomJid = args[1] + "@" + m_conferenceHost;
		CXMPPRoom *room    = findRoom( roomJid );
		QString    message = "";
		if ( room ) {
			for ( int i = 2; i < args.size(); i++ ) {
				if ( message != "" ) message += " ";
				message += args[i];
			}
			room->say( message );
		}

		return;
	}

	if ( args[0] == "runscript" ) {
		if ( !checkLoggedIn( msg.from() ) ) {
			m_client->sendMessage( msg.from(), "You are not logged in" );
			return;
		}
		if ( args.size() < 3 ) {
			m_client->sendMessage( msg.from(), "Usage: runscript <room> <script>" );
			return;
		}

		int idx = args[2].lastIndexOf( '/' );
		if ( idx >= 0 ) args[2] = args[2].right( idx );
		QString    roomJid = args[1] + "@" + m_conferenceHost;
		CXMPPRoom *room    = findRoom( roomJid );
		if ( room ) {
			CScriptController *script = room->findScript( args[2] );
			if ( !script ) {
				room->runScript( args[2] );
			} else {
				if ( !script->running() ) script->runScript();
			}
		}

		return;
	}

	if ( args[0] == "stopscript" ) {
		if ( !checkLoggedIn( msg.from() ) ) {
			m_client->sendMessage( msg.from(), "You are not logged in" );
			return;
		}
		if ( args.size() < 3 ) {
			m_client->sendMessage( msg.from(), "Usage: stopscript <room> <script>" );
			return;
		}

		int idx = args[2].lastIndexOf( '/' );
		if ( idx >= 0 ) args[2] = args[2].right( idx );
		QString    roomJid = args[1] + "@" + m_conferenceHost;
		CXMPPRoom *room    = findRoom( roomJid );
		if ( room ) {
			CScriptController *script = room->findScript( args[2] );
			if ( script ) {
				if ( script->running() ) script->stopScript();
			}
		}

		return;
	}

	if ( args[0] == "autorun" ) {
		if ( !checkLoggedIn( msg.from() ) ) {
			m_client->sendMessage( msg.from(), "You are not logged in" );
			return;
		}
		if ( args.size() < 3 ) {
			m_client->sendMessage( msg.from(), "Usage: runscript <room> <script>" );
			return;
		}

		int idx = args[2].lastIndexOf( '/' );
		if ( idx >= 0 ) args[2] = args[2].right( idx );
		QString    roomJid = args[1] + "@" + m_conferenceHost;
		CXMPPRoom *room    = findRoom( roomJid );
		if ( room ) {
			CScriptController *script = room->findScript( args[2] );
			if ( script ) {
				script->toggleAutoRun();
				if ( script->autoRun() )
					m_client->sendMessage( msg.from(), fmt( "AUTORUN flag on script '%1' in room '%2' is now ON" ).arg( script->script() ).arg( room->roomName() ) );
				else
					m_client->sendMessage( msg.from(), fmt( "AUTORUN flag on script '%1' in room '%2' is now OFF" ).arg( script->script() ).arg( room->roomName() ) );
			}
		}

		return;
	}

	if ( args[0] == "quit" ) {
		if ( !checkLoggedIn( msg.from() ) ) {
			m_client->sendMessage( msg.from(), "You are not logged in" );
			return;
		}
		LOG( fmt( "Shutdown initiated by user '%1 (%2)'" ).arg( m_loggedInUsers[msg.from()] ).arg( msg.from() ) );
		botInstance->quit();
		return;
	}
}

void CXMPPServer::messageReceived( const QXmppMessage &msg )
{
	switch ( msg.type() ) {
		case QXmppMessage::Normal:
		case QXmppMessage::Chat: {
			handleBotCommand( msg );
			break;
		}
		case QXmppMessage::GroupChat: {
			QString bareJid = QXmppUtils::jidToBareJid( msg.from() );
			CXMPPRoom *room = findRoom( bareJid );
			if ( !msg.stamp().isValid() && room )
				room->handleRoomMessage( msg );
			break;
		}
		default: {
			break;
		}
	}
}

void CXMPPServer::presenceReceived( const QXmppPresence &presence )
{
	CXMPPRoom *room = findRoom( QXmppUtils::jidToBareJid( presence.from() ) );
	if ( room )
		room->handleRoomPresence( presence);
}
