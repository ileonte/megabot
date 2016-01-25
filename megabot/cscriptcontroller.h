#ifndef __CSCRIPTCONTROLLER_H_INCLUDED__
#define __CSCRIPTCONTROLLER_H_INCLUDED__

#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>

#include <QObject>
#include <QLocalSocket>

#include "ctlpackets.h"

class CXMPPRoom;
class CMegaBot;

class CScriptController : public QObject
{
	Q_OBJECT

private:
	bool          m_running;
	bool          m_autoRun;
	bool          m_stopping;
	int           m_sockfds[2];
	QLocalSocket *m_comm;
	CXMPPRoom    *m_room;
	QString       m_script;
	pid_t         m_child;
	QByteArray    m_sockData;

	void closeSockets();

	void sendPacket( const CBaseControlPacket &pkt );
	void waitForChild();

private slots:
	void socketDisconnected();
	void socketReadyRead();

public:
	CScriptController( CXMPPRoom *parent, const QString &script );
	virtual ~CScriptController();

	bool autoRun() const { return m_autoRun; }
	void setAutoRun( bool yesno ) { m_autoRun = yesno; }
	void toggleAutoRun() { m_autoRun = !m_autoRun; }

	bool running() const { return m_running; }

	const CXMPPRoom *room() const { return m_room; }

	QString script() const { return m_script; }

	bool runScript();
	void stopScript();

	friend class CMegaBot;

signals:
	void stopped( CScriptController *controller );
	void message( const CScriptMessagePacket &msg );

public slots:
	void sendRoomConfig();
	void sendRoomMessage( const QXmppMessage &msg );
	void sendRoomPresence( const QXmppPresence &presence );

	QString logHandle() { return fmt( "%1/%2" ).arg( m_room->logHandle() ).arg( m_script ); }
};

#endif // __CSCRIPTCONTROLLER_H_INCLUDED__
