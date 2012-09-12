#ifndef __CROOMCONFIG_H_INCLUDED__
#define __CROOMCONFIG_H_INCLUDED__

#include "cxmppserver.h"
#include "main.h"

class CScriptController;
class CScriptMessagePacket;

class CXMPPRoom : public QObject
{
	Q_OBJECT

private:
	QString                     m_roomName;
	QString                     m_nickName;
	QString                     m_password;
	CXMPPServer                *m_server;
	QXmppMucManager            *m_mucman;
	QXmppMucRoom               *m_mucroom;
	bool                        m_autoJoin;
	QList<QString>              m_participants;
	QList<CScriptController *>  m_scripts;

private slots:
	void scriptMessage( const CScriptMessagePacket &pkt );
	void roomJoined();

public:
	CXMPPRoom( CXMPPServer *parent );
	~CXMPPRoom();

	bool isEmpty() const {
		return ( m_roomName.isEmpty() || m_nickName.isEmpty() );
	}

	QString roomName() const { return m_roomName; }
	void setRoomName( const QString &v ) { part(); m_roomName = v; }

	QString nickName() const { return m_nickName; }
	void setNickName( const QString &v ) { part(); m_nickName = v; }

	QString password() const { return m_password; }
	void setPassword( const QString &v ) { part(); m_password = v; }

	bool autoJoin() const { return m_autoJoin; }
	void setAutoJoin( bool yesno ) { m_autoJoin = yesno; }
	void toggleAutoJoin() { m_autoJoin = !m_autoJoin; }

	CXMPPServer *server() const { return m_server; }

	QString jid() const { return m_roomName + "@" + m_server->conferenceHost() + "/" + m_nickName; }
	QString bareJid() const { return m_roomName + "@" + m_server->conferenceHost(); }

	bool join();
	bool joined() { return m_mucroom ? m_mucroom->isJoined() : false; }
	void part();

	CScriptController *findScript( const QString &name );
	bool runScript( const QString &name );

	void addScript( CScriptController *script );

	friend class CMegaBot;

public slots:
	void handleRoomPresence( const QXmppPresence &presence );
	void handleRoomMessage( const QXmppMessage &msg );

	void say( const QString &what ) { if ( m_mucroom ) m_mucroom->sendMessage( what ); }

	QString logHandle() { return fmt( "%1/%2" ).arg( m_server->logHandle() ).arg( m_roomName ); }
};

#endif  /* __CROOMCONFIG_H_INCLUDED__ */
