#include "cmegabot.h"
#include "cxmpproom.h"
#include "cscriptcontroller.h"

CScriptController::CScriptController(CXMPPRoom *parent, const QString &script) : QObject(parent)
{
	m_room = parent;
	m_script = script;
	m_running = false;
	m_comm = NULL;
	m_child = -1;
	m_autoRun = false;
}

CScriptController::~CScriptController()
{
	if (m_running && m_child > 0) {
		m_comm->disconnectFromServer();
		waitForChild();
		closeSockets();
	}
}

void CScriptController::waitForChild()
{
	int status;
	LOG(fmt("Waiting for child '%1' to terminate").arg(m_child));
	waitpid(m_child, &status, 0);
	m_child = -1;

	if (WIFEXITED(status)) {
		LOG(fmt("Script has exited with code %1").arg(WEXITSTATUS(status)));
	} else if (WIFSIGNALED(status)) {
		int sig = WTERMSIG(status);
		LOG(fmt("Script was terminated by signal %1 (%2)%3")
		        .arg(sig)
		        .arg(strsignal(sig))
		        .arg(WCOREDUMP(status) ? ". Core dumped" : ""));
	} else {
		LOG(fmt("Script has terminated unexpectedly"));
	}
}

void CScriptController::closeSockets()
{
	shutdown(m_sockfds[0], SHUT_RDWR);
	close(m_sockfds[0]);
	shutdown(m_sockfds[1], SHUT_RDWR);
	close(m_sockfds[1]);
	m_sockfds[0] = m_sockfds[1] = -1;
}

bool CScriptController::runScript()
{
	if (m_child != (pid_t)-1)
		return true;

	if (socketpair(AF_UNIX, SOCK_STREAM, 0, m_sockfds) < 0) {
		LOG(fmt("socketpair(): %1").arg(strerror(errno)));
		return false;
	}

	m_child = fork();
	if (m_child < 0) {
		closeSockets();
		LOG(fmt("fork(): %1").arg(strerror(errno)));
		return false;
	}

	if (m_child) {
		/* parent */
		close(m_sockfds[1]);
		m_sockfds[1] = -1;

		m_comm = new QLocalSocket(this);
		m_comm->setSocketDescriptor(m_sockfds[0], QLocalSocket::ConnectedState);
		m_running = true;

		connect(m_comm, SIGNAL(disconnected()), this, SLOT(socketDisconnected()));
		connect(m_comm, SIGNAL(readyRead()), this, SLOT(socketReadyRead()));

		CRoomConfigPacket pkt(m_room);
		sendPacket(pkt);
	} else {
		/* child */
		close(m_sockfds[0]);
		botInstance->closeAllSockets(m_sockfds[1]);

		setenv("MEGABOT_CONTROL_SOCKET", fmt("%1").arg(m_sockfds[1]).toUtf8().data(), 1);
		setenv("MEGABOT_SERVER", m_room->server()->conferenceHost().toUtf8().data(), 1);
		setenv("MEGABOT_SERVER_HANDLE", m_room->server()->logHandle().toUtf8().data(), 1);
		setenv("MEGABOT_HANDLE", logHandle().toUtf8().data(), 1);
		setenv("MEGABOT_ROOM", m_room->roomName().toUtf8().data(), 1);
		setenv("MEGABOT_NICKNAME", m_room->nickName().toUtf8().data(), 1);
		setenv("MEGABOT_BASEPATH", botInstance->basePath().toUtf8().data(), 1);
		setenv("MEGABOT_SCRIPT", m_script.toUtf8().data(), 1);
		QString pname = QString("%1 %2").arg(MB_SCRIPT_RUNNER_NAME).arg(logHandle());
		char *args[] = {strdup((char *)pname.toUtf8().data()), NULL};
		execvp(botInstance->applicationFilePath().toUtf8().data(), args);

		LOG(fmt("Failed to execute script process: %1").arg(strerror(errno)));
		botInstance->exit(1);
		return false;
	}

	return true;
}

void CScriptController::stopScript()
{
	if (m_running && m_child > 0) {
		LOG(fmt("Stopping script '%1'").arg(m_script));
		if (m_comm) {
			LOG(fmt("Closing socket connection"));
			m_comm->disconnectFromServer();
			m_comm = NULL;
			LOG(fmt("DONE Closing socket connection"));
		}
	}
}

void CScriptController::socketDisconnected()
{
	m_sockData.clear();
	closeSockets();
	if (m_child > 0)
		waitForChild();

	emit stopped(this);

	m_running = false;
}

void CScriptController::socketReadyRead()
{
	m_sockData.append(m_comm->readAll());

	while (1) {
		CScriptMessagePacket pkt;

		if (!pkt.unpack(m_sockData))
			break;

		emit message(pkt);
	}
}

void CScriptController::sendRoomConfig()
{
	CRoomConfigPacket pkt(m_room);
	sendPacket(pkt);
}

void CScriptController::sendRoomMessage(const QXmppMessage &msg)
{
	CRoomMessagePacket pkt(msg);
	sendPacket(pkt);
}

void CScriptController::sendRoomPresence(const QXmppPresence &presence)
{
	CRoomPresencePacket pkt(presence);
	sendPacket(pkt);
}

void CScriptController::sendPacket(const CBaseControlPacket &pkt)
{
	if (m_running) {
		QByteArray data;
		pkt.pack(data);
		m_comm->write(data);
	}
}
