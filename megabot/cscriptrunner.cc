#include "cscriptrunner.h"
#include "cluarunner.h"
#include "ctlpackets.h"

CScriptRunnerBase::CScriptRunnerBase(const QString &handle, const QString &name, int fd, const QVariantMap &extraConfig,
                                     QObject *parent)
    : QObject(parent), m_extraConfig(extraConfig)
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
