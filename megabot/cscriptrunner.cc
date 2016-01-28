#include "cscriptrunner.h"
#include "cluarunner.h"
#include "ctlpackets.h"

#include <QHostAddress>

CScriptRunnerBase::CScriptRunnerBase(const QString &handle, const QString &name, int fd, const QVariantMap &extraConfig,
                                     QObject *parent)
    : QObject(parent), m_allowListen(true), m_extraConfig(extraConfig)
{
	m_networkRequestCount = 0;
	m_timerCount = 0;

	m_handle = handle;
	m_script = name;
	m_comm = new QLocalSocket(this);
	if (!m_comm->setSocketDescriptor(fd))
		LOG(fmt("Warning: FAILED to create QLocalSocket on top of fd %1, "
		        "stuff will break !")
		        .arg(fd));

	QVariant vv = m_extraConfig.value("allowLocalServers");
	if (vv.isValid() && vv.type() == QVariant::Bool)
		m_allowListen = vv.toBool();

	QVariantMap env = m_extraConfig.value("env").toMap();
	foreach (QString valName, env.keys()) {
		QVariant val = env.value(valName);
		if (val.type() == QVariant::String)
			setenv(valName.toUtf8().data(), val.toString().toUtf8().data(), 1);
	}

	m_netMan = new QNetworkAccessManager(this);
	connect(m_netMan, SIGNAL(finished(QNetworkReply *)), this, SLOT(networkRequestFinished(QNetworkReply *)));

	connect(m_comm, SIGNAL(disconnected()), this, SLOT(socketDisconnected()));
	connect(m_comm, SIGNAL(readyRead()), this, SLOT(socketReadyRead()));
}

CScriptRunnerBase::~CScriptRunnerBase()
{
}

void CScriptRunnerBase::socketReadyRead()
{
	m_sockData.append(m_comm->readAll());

	while (1) {
		if (m_sockData.size() < 4)
			break;

		qint32 t = *((qint32 *)(m_sockData.data()));
		CBaseControlPacket::ControlPatcketType type = (CBaseControlPacket::ControlPatcketType)t;
		switch (type) {
			case CBaseControlPacket::RoomConfig: {
				CRoomConfigPacket pkt;
				if (!pkt.unpack(m_sockData))
					return;
				onRoomConfigPacket(pkt);
				break;
			}
			case CBaseControlPacket::RoomMessage: {
				CRoomMessagePacket pkt;
				if (!pkt.unpack(m_sockData))
					return;
				onRoomMessagePacket(pkt);
				break;
			}
			case CBaseControlPacket::RoomPresence: {
				CRoomPresencePacket pkt;
				if (!pkt.unpack(m_sockData)) {
					LOG(fmt("Failed to unpack romm presence packet"));
					return;
				}
				onRoomPresencePacket(pkt);
				break;
			}
			default: {
				LOG(fmt("Got invalid message type %1, exiting").arg(t));
				botInstance->quit();
				return;
			}
		}
	}
}

void CScriptRunnerBase::socketDisconnected()
{
	LOG("Control socket disconnected, quitting");
	botInstance->quit();
	return;
}

void CScriptRunnerBase::networkRequestFinished(QNetworkReply *reply)
{
	QByteArray data;
	bool allOk = true;

	if (reply->error() != QNetworkReply::NoError) {
		data.append(reply->errorString().toUtf8());
		allOk = false;
	} else {
		data.append(reply->readAll());
		allOk = true;
	}

	m_networkRequestCount -= 1;

	onNetworkRequestFinished(allOk, reply->objectName(), reply->request().url().toString(), data);
	reply->deleteLater();
}

void CScriptRunnerBase::timerTimeout()
{
	QTimer *timer = dynamic_cast<QTimer *>(sender());
	if (timer) {
		m_timerCount -= 1;
		onTimerTimeout(timer->objectName());
		disconnect(timer, SIGNAL(timeout()), this, SLOT(timerTimeout()));
		timer->deleteLater();
	}
}

void CScriptRunnerBase::onNetworkRequestFinished(bool, const QString &, const QString &, const QByteArray &)
{
}

void CScriptRunnerBase::onRoomConfigPacket(const CRoomConfigPacket &pkt)
{
	m_roomJid = pkt.roomJid();
	m_roomName = pkt.roomName();
	m_nickName = pkt.nickName();
}

void CScriptRunnerBase::onRoomMessagePacket(const CRoomMessagePacket &)
{
}
void CScriptRunnerBase::onRoomPresencePacket(const CRoomPresencePacket &pkt)
{
	if (pkt.presenceType() == QXmppPresence::Available)
		if (!m_participants.contains(pkt.who()))
			m_participants.append(pkt.who());
	if (pkt.presenceType() == QXmppPresence::Unavailable)
		m_participants.removeAll(pkt.who());
}

void CScriptRunnerBase::onTimerTimeout(const QString &)
{
}
void CScriptRunnerBase::sendMessage(const QString &to, const QString &body, const QString &subject, bool fixedFont)
{
	CScriptMessagePacket pkt(to, body, subject, fixedFont);
	QByteArray data;
	pkt.pack(data);
	m_comm->write(data);
}

void CScriptRunnerBase::networkRequest(const QString &name, const QString &url)
{
	QString rname(name);
	if (rname.isEmpty()) {
#if QT_VERSION < 0x040700
		rname = QString("netMan_request_%1").arg(QDateTime::currentDateTime().toTime_t());
#else
		rname = QString("netMan_request_%1").arg(QDateTime::currentMSecsSinceEpoch());
#endif
	}

	QUrl u(url);
	if (!u.isValid()) {
		onNetworkRequestFinished(false, rname, url, u.errorString().toUtf8());
		return;
	}

	if (m_networkRequestCount >= MB_MAX_NETREQ_COUNT) {
		onNetworkRequestFinished(false, rname, url, QString("Too many network requests already running").toUtf8());
		return;
	}

	QNetworkRequest req(u);
	req.setRawHeader("User-Agent", "Mozilla/5.0 (X11; U; Linux x86_64; "
	                               "en-US) AppleWebKit/534.16 (KHTML, like "
	                               "Gecko) Chrome/10.0.648.127 "
	                               "Safari/534.16");
	QNetworkReply *reply = m_netMan->get(req);
	reply->setObjectName(rname);
	m_networkRequestCount += 1;
}

void CScriptRunnerBase::createTimer(const QString &name, int timeout)
{
	if (m_timerCount < MB_MAX_TIMER_COUNT && timeout > 0) {
		QTimer *timer = new QTimer(this);
		QString tname(name.trimmed());
		if (tname.isEmpty()) {
#if QT_VERSION < 0x040700
			tname = QString("mb_timer_%1").arg(QDateTime::currentDateTime().toTime_t());
#else
			tname = QString("mb_timer_%1").arg(QDateTime::currentMSecsSinceEpoch());
#endif
		}
		timer->setSingleShot(true);
		timer->setInterval(timeout);
		timer->setObjectName(tname);
		connect(timer, SIGNAL(timeout()), this, SLOT(timerTimeout()));
		m_timerCount += 1;
		timer->start();
	}
}

void CScriptRunnerBase::handleTcpConnectionData()
{
	QTcpSocket *client = qobject_cast<QTcpSocket *>(sender());
	if (!client)
		return;
	QTcpServer *srv = qobject_cast<QTcpServer *>(client->parent());
	if (!srv)
		return;

	while (!client->atEnd()) {
		onTcpConnectionData(srv, client, client->read(65535));
	}
}

void CScriptRunnerBase::handleTcpConnectionClose()
{
	QTcpSocket *client = qobject_cast<QTcpSocket *>(sender());
	if (!client)
		return;
	QTcpServer *srv = qobject_cast<QTcpServer *>(client->parent());
	if (!srv)
		return;

	onTcpConnectionClosed(srv, client);
}

void CScriptRunnerBase::handleNewTcpConnection()
{
	QTcpServer *srv = qobject_cast<QTcpServer *>(sender());
	while (srv->hasPendingConnections()) {
		QTcpSocket *sock = srv->nextPendingConnection();
		QString sockName;
#if QT_VERSION < 0x040700
		sockName = QString("%1-%2").arg(srv->objectName()).arg(QDateTime::currentDateTime().toTime_t());
#else
		sockName = QString("%1-%21").arg(srv->objectName()).arg(QDateTime::currentMSecsSinceEpoch());
#endif
		sock->setObjectName(sockName);

		if (!onNewTcpConnection(srv, sock)) {
			sock->close();
			sock->deleteLater();
			continue;
		}

		connect(sock, SIGNAL(readyRead()), this, SLOT(handleTcpConnectionData()));
		connect(sock, SIGNAL(disconnected()), this, SLOT(handleTcpConnectionClose()));
	}
}

void CScriptRunnerBase::localServerCreate(const QString &name, quint16 port, QTcpServer **server, QString &error)
{
	*server = 0;
	if (!m_allowListen) {
		error = "Creating listen servers is not allowed";
		return;
	}
	if (name.isEmpty()) {
		error = "You failed to specify a name for the server";
		return;
	}
	if (findChild<QTcpServer *>(name)) {
		error = "A server with that name already exists";
		return;
	}

	QTcpServer *srv = new QTcpServer(this);
	srv->setObjectName(name);
	if (!srv->listen(QHostAddress::LocalHost, port)) {
		error = srv->errorString();
		srv->deleteLater();
		return;
	}

	connect(srv, SIGNAL(newConnection()), this, SLOT(handleNewTcpConnection()));
	*server = srv;
}

void CScriptRunnerBase::localServerDestroy(const QString &name)
{
	QTcpServer *srv = findChild<QTcpServer *>(name);
	if (srv) {
		srv->close();
		srv->deleteLater();
	}
}

void CScriptRunnerBase::localServerDestroyClient(const QString &name, const QString &clientName)
{
	QTcpServer *srv = findChild<QTcpServer *>(name);
	if (!srv)
		return;
	QTcpSocket *cli = srv->findChild<QTcpSocket *>(clientName);
	if (!cli)
		return;
	cli->close();
	cli->deleteLater();
}

void CScriptRunnerBase::localClientDestroy(const QString &name)
{
	QTcpSocket *cli = findChild<QTcpSocket *>(name);
	if (cli) {
		cli->close();
		cli->deleteLater();
	}
}

qint64 CScriptRunnerBase::localServerSend(const QString &name, const QString &clientName, const char *data, qint64 size, QString &error)
{
	QTcpServer *srv = findChild<QTcpServer *>(name);
	if (!srv) {
		error = "No such server found";
		return -1;
	}
	QTcpSocket *cli = srv->findChild<QTcpSocket *>(clientName);
	if (!cli) {
		error = "No such client found";
		return -1;
	}

	qint64 ret = cli->write(data, size);
	if (ret < 0) {
		error = cli->errorString();
		return -1;
	}

	return ret;
}

qint64 CScriptRunnerBase::localClientSend(const QString &name, const char *data, qint64 size, QString &error)
{
	QTcpSocket *cli = findChild<QTcpSocket *>(name);
	if (!cli) {
		error = "No such client found";
		return -1;
	}

	qint64 ret = cli->write(data, size);
	if (ret < 0) {
		error = cli->errorString();
		return -1;
	}

	return ret;
}
