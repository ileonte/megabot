#ifndef __CSERVERCONFIG_H_INCLUDED__
#define __CSERVERCONFIG_H_INCLUDED__

#include "main.h"

class CXMPPRoom;

class CXMPPServer : public QObject
{
	Q_OBJECT

private:
	QString    m_handle;
	QString    m_host;
	QString    m_domain;
	QString    m_account;
	QString    m_resource;
	QString    m_password;
	QString    m_conferenceHost;

	bool       m_userDisconnect;

	QList<CXMPPRoom *>      m_rooms;
	QXmppClient            *m_client;
	QXmppMucManager        *m_mucman;
	QMap<QString, QString>  m_loggedInUsers;

	bool checkUsernameAndPassword( const QString &username, const QString &password );
	bool checkLoggedIn( const QString &jid ) const;
	void handleBotCommand( const QXmppMessage &msg );

private slots:
	void clientConnected();
	void clientDisconnected();
	void messageReceived( const QXmppMessage &msg );
	void presenceReceived( const QXmppPresence &presence );
	void qxmppLogEvent( const QXmppLogger::MessageType &type, const QString &msg );

public:
	CXMPPServer( const QString &handle, QObject *parent = 0 );
	~CXMPPServer( void );

	bool isEmpty() const {
		return ( m_host.isEmpty() ||
			 m_domain.isEmpty() ||
			 m_account.isEmpty() ||
			 m_resource.isEmpty() ||
			 m_password.isEmpty() );
	}

	QString jid() const {
		return m_account + "@" + m_host + "/" + m_resource;
	}
	QString bareJid() const {
		return m_account + "@" + m_host;
	}
	bool setJid( const QString &jid );

	QString handle() const { return m_handle; }

	QString host() const { return m_host; }
	void setHost( const QString &v ) { m_host = v; m_domain = v; }

	QString domain() const { return m_domain; }
	void setDomain( const QString &v ) { m_domain = v; }

	QString account() const { return m_account; }
	void setAccount( const QString &v ) { m_account = v; }

	QString resource() const { return m_resource; }
	void setResource( const QString &v ) { m_resource = v; }

	QString password() const { return m_password; }
	void setPassword( const QString &v ) { m_password = v; }

	QString conferenceHost() const { return m_conferenceHost; }
	void setConferenceHost( const QString &v ) { m_conferenceHost = v; }

	void addRoom( CXMPPRoom *room ) { m_rooms.append( room ); }
	CXMPPRoom *findRoom( const QString &bareJid );

	friend class CXMPPRoom;
	friend class CMegaBot;

public slots:
	void connectToServer();
	void disconnectFromServer();

	QString logHandle() { return m_handle; }
};

#endif  /* __CSERVERCONFIG_H_INCLUDED__ */
