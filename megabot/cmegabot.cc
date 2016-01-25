#include "cxmppserver.h"
#include "cxmpproom.h"
#include "cmegabot.h"
#include "cscriptrunner.h"
#include "cluarunner.h"
#include "cscriptcontroller.h"
#include "cjsonparser.h"

CMegaBot::CMegaBot(int &argc, char **argv) : QCoreApplication(argc, argv)
{
	m_mode = Unknown;
	m_forked = false;
	m_runner = NULL;

	qRegisterMetaType<QAbstractSocket::SocketState>();
}

CMegaBot::~CMegaBot()
{
	m_servers.clear();
}
void CMegaBot::triggerKillSwitch()
{
	QLocalSocket ks;

	ks.connectToServer(MB_KILL_SWITCH);
	if (!ks.waitForConnected(-1)) {
		LOG(fmt("Failed to trigger kill-switch: %1").arg(ks.errorString()));
		return;
	}

	ks.write("quit");
	ks.waitForBytesWritten(-1);

	ks.disconnectFromServer();
}

void CMegaBot::dataOnKillSwitchConnection()
{
	LOG("Local kill-switch activated");
	quit();
}

void CMegaBot::newKillSwitchConnection()
{
	QLocalSocket *conn = NULL;

	while ((conn = m_killSwitch->nextPendingConnection()) != NULL) {
		conn->setParent(this);
		connect(conn, SIGNAL(readyRead()), this, SLOT(dataOnKillSwitchConnection()));
	}
}

bool CMegaBot::loadConfig()
{
	QFileInfo fi[] = {QString("/etc/MegaBot/config.json"), m_basePath + "/etc/config.json"};
	QStringList errors;

	for (unsigned i = 0; i < sizeof(fi) / sizeof(fi[0]); i++) {
		CJSONParser p;

		if (!p.parse(fi[i].absoluteFilePath())) {
			if (p.error().line > 0) {
				errors.append(fmt("Failed to parse '%1': at line %2:%3 - %4")
				                  .arg(fi[i].absoluteFilePath())
				                  .arg(p.error().line)
				                  .arg(p.error().column)
				                  .arg(p.error().message));
			} else {
				errors.append(fmt("Failed to parse '%1': %2").arg(fi[i].absoluteFilePath()).arg(p.error().message));
			}
			continue;
		}
		m_config = p.value().toMap();

		m_configPath = fi[i].absoluteFilePath();
		LOG(fmt("Successfully loaded config from %1").arg(m_configPath));
		return true;
	}

	foreach (const QString &err, errors)
		LOG(err);
	return false;
}

void CMegaBot::initPaths(const QString &basePath)
{
	if (!basePath.isEmpty())
		m_basePath = basePath;
	else
		m_basePath = "/opt/MegaBot";

	m_scriptPath = fmt("%1/share/scripts").arg(m_basePath);
	m_logPath = fmt("%1/var/log").arg(m_basePath);
}

bool CMegaBot::initMaster(bool dofork, const QString &basePath)
{
	m_mode = Master;

	initPaths(basePath);

	if (!loadConfig())
		return false;

	m_killSwitch = new QLocalServer(this);
	if (!m_killSwitch->listen(MB_KILL_SWITCH)) {
		LOG(fmt("Failed to create MegaBot kill switch: %1").arg(m_killSwitch->errorString()));
		delete m_killSwitch;
		return false;
	}
	connect(m_killSwitch, SIGNAL(newConnection()), this, SLOT(newKillSwitchConnection()));

	if (dofork) {
		if (daemon(1, 0) < 0) {
			LOG(fmt("Failed to daemonize: %1").arg(strerror(errno)));
			return false;
		}
		m_forked = true;
	}

	foreach (const QVariant &vSrvName, m_config["servers"].toMap().keys()) {
		QString server_handle = vSrvName.toString();
		if (server_handle.isEmpty())
			continue;

		QVariantMap mSrv = m_config["servers"].toMap().value(server_handle).toMap();

		CXMPPServer *server = new CXMPPServer(server_handle, this);
		server->setHost(mSrv["host"].toString());
		server->setDomain(mSrv["domain"].toString());
		server->setAccount(mSrv["account"].toString());
		server->setResource(mSrv["resource"].toString());
		server->setPassword(mSrv["password"].toString());
		server->setConferenceHost(mSrv["conferenceHost"].toString());
		if (server->isEmpty()) {
			delete server;
			continue;
		}

		foreach (const QString &roomName, mSrv["rooms"].toMap().keys()) {
			QVariantMap mRoom = mSrv["rooms"].toMap().value(roomName).toMap();

			CXMPPRoom *room = new CXMPPRoom(server);
			room->setRoomName(roomName);
			room->setNickName(mRoom["nickName"].toString());
			room->setPassword(mRoom["password"].toString());
			room->setAutoJoin(true);
			if (room->isEmpty()) {
				delete room;
				continue;
			}

			foreach (const QString &script_name, mRoom["scripts"].toMap().keys()) {
				CScriptController *script = room->findScript(script_name);
				if (!script) {
					script = new CScriptController(room, script_name);
					script->setAutoRun(true);
					room->addScript(script);
				}
			}

			server->addRoom(room);
		}

		m_servers.append(server);
		connect(this, SIGNAL(botInitialized()), server, SLOT(connectToServer()));
	}

	if (!m_servers.size()) {
		LOG("No valid server configuration found");
		return false;
	}

	emit botInitialized();

	return true;
}

bool CMegaBot::createRunner(const QString &handle, const QString &name, int fd, const QVariantMap &extraConfig)
{
	if (name.endsWith(".lua", Qt::CaseInsensitive))
		m_runner = new CLuaRunner(handle, name, fd, extraConfig, this);
	else
		return false;

	if (!m_runner->setupScript()) {
		delete m_runner;
		m_runner = 0;
		return false;
	}

	return true;
}

bool CMegaBot::initScriptRunner()
{
	QMap<QString, QString> varMap;
	QString error;
	QString script;
	QString server;
	QString serverHandle;
	QString handle;
	QString room;
	QString nickname;

	m_mode = ScriptRunner;
	m_forked = true;

	varMap["MEGABOT_CONTROL_SOCKET"] = "";
	varMap["MEGABOT_SERVER"] = "";
	varMap["MEGABOT_SERVER_HANDLE"] = "";
	varMap["MEGABOT_HANDLE"] = "";
	varMap["MEGABOT_ROOM"] = "";
	varMap["MEGABOT_NICKNAME"] = "";
	varMap["MEGABOT_BASEPATH"] = "";
	varMap["MEGABOT_SCRIPT"] = "";

	foreach (const QString &var, varMap.keys()) {
		varMap[var] = CMegaBot::getEnv(var);
		if (varMap[var].isEmpty())
			error += " " + var;
	}
	if (!error.isEmpty()) {
		qDebug() << fmt("Missing vars: %1").arg(error);
		LOG(fmt("Missing vars: %1").arg(error));
		return false;
	}

	bool ok = true;
	int fd = varMap["MEGABOT_CONTROL_SOCKET"].toInt(&ok);
	if (!ok || (ok && fd < 0))
		return false;

	initPaths(varMap["MEGABOT_BASEPATH"]);

	script = varMap["MEGABOT_SCRIPT"];
	server = varMap["MEGABOT_SERVER"];
	serverHandle = varMap["MEGABOT_SERVER_HANDLE"];
	handle = varMap["MEGABOT_HANDLE"];
	room = varMap["MEGABOT_ROOM"];
	nickname = varMap["MEGABOT_NICKNAME"];

	if (!loadConfig())
		return false;

	QVariant scriptConfig =
	    m_config["servers"].toMap()[serverHandle].toMap()["rooms"].toMap()[room].toMap()["scripts"].toMap().value(script);
	if (!createRunner(handle, script, fd, scriptConfig.toMap()))
		return false;
	m_runner->setInitialConfig(server, room, nickname);

	return true;
}

void CMegaBot::quit()
{
	if (m_mode == Master) {
		for (int i = 0; i < m_servers.size(); i++)
			m_servers[i]->disconnectFromServer();
	}
	QCoreApplication::quit();
}

void CMegaBot::closeAllSockets(int except)
{
	for (int i = 0; i < m_servers.size(); i++) {
		CXMPPServer *server = m_servers[i];
		for (int j = 0; j < server->m_rooms.size(); j++) {
			CXMPPRoom *room = server->m_rooms[j];
			for (int k = 0; k < room->m_scripts.size(); k++) {
				CScriptController *script = room->m_scripts[k];
				if (script->m_sockfds[0] != except)
					close(script->m_sockfds[0]);
				if (script->m_sockfds[1] != except)
					close(script->m_sockfds[1]);
			}
		}
	}
}
