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
#include <QFileInfo>
#include <QVariant>

#include <json.h>

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

#ifndef PNU
#define PNU __attribute__(( unused ))
#endif

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
	QString               m_scriptPath;
	QString               m_logPath;

	QString               m_configPath;
	QVariantMap           m_config;
	QList<CXMPPServer *>  m_servers;

	CScriptRunnerBase    *m_runner;

	QLocalServer         *m_killSwitch;

private slots:
	void newKillSwitchConnection();
	void dataOnKillSwitchConnection();

	bool loadConfig();

public:
	CMegaBot( int &argc, char **argv );
	~CMegaBot( void );

	void closeAllSockets( int except );

	bool initMaster( bool dofork, const QString &basePath = "" );
	bool initScriptRunner();

	bool forked() { return m_forked; }

	void quit();

	void writeDummyConfig( const QString &path );

	QString basePath() const { return m_basePath; }
	QString scriptPath() const { return m_scriptPath; }
	QString logPath() const { return m_logPath; }

	CScriptRunnerBase *scriptRunner() { return m_runner; }

	void triggerKillSwitch();

	static QString getEnv( const QString &var ) {
		char *v = getenv( var.toUtf8().data() );
		return QString( v ? v : "" ).trimmed();
	}

	QString configPath() { return m_configPath; }
	const QVariantMap &config() { return m_config; }

signals:
	void botInitialized();

public slots:
	QString logHandle() { return QString( "megabot" ); }
};

#define botInstance static_cast<CMegaBot *>( qApp )

#endif // __CMEGABOT_H_INCLUDED__
