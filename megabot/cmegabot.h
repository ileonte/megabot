#ifndef __CMEGABOT_H_INCLUDED__
#define __CMEGABOT_H_INCLUDED__

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

#include <QCoreApplication>
#include <QLocalSocket>
#include <QLocalServer>
#include <QSettings>
#include <QFile>
#include <QList>
#include <QMap>

#include <QXmppUtils.h>
#include <QXmppClient.h>
#include <QXmppLogger.h>
#include <QXmppMessage.h>
#include <QXmppPresence.h>
#include <QXmppMucManager.h>
#include <QXmppConfiguration.h>
#include <QXmppReconnectionManager.h>

#define MB_SCRIPT_RUNNER_NAME "[megabot-script]"
#define MB_KILL_SWITCH "/tmp/mb_kill_switch"

class CXMPPServer;
class CScriptRunnerBase;

class CMegaBot : public QCoreApplication
{
	Q_OBJECT

private:
	enum Mode {
		Unknown,
		Master,
		ScriptRunner
	};

	Mode                  m_mode;
	bool                  m_forked;
	QString               m_basePath;
	QString               m_logName;
	QList<CXMPPServer *>  m_servers;

	CScriptRunnerBase    *m_runner;

	QLocalServer         *m_killSwitch;

private slots:
	void newKillSwitchConnection();
	void dataOnKillSwitchConnection();

public:
	CMegaBot( int &argc, char **argv );
	~CMegaBot( void );

	void closeAllSockets( int except );

	bool initMaster( bool dofork );
	bool initScriptRunner( const QString &script, int fd );

	void quit();

	void writeDummyConfig( const QString &path );

	QString basePath() const { return m_basePath; }

	CScriptRunnerBase *scriptRunner() { return m_runner; }

	void triggerKillSwitch();

signals:
	void configLoaded();

public slots:
	void log( int line, const QString &file, const QString &message );
	void qxmppLogEvent( const QXmppLogger::MessageType &type, const QString &msg );
};

#define myApp static_cast<CMegaBot *>( qApp )

#endif // __CMEGABOT_H_INCLUDED__
