#ifndef __CSCRIPTRUNNER_H_INCLUDED__
#define __CSCRIPTRUNNER_H_INCLUDED__

#include <QObject>
#include <QStringList>
#include <QLocalSocket>
#include <QTcpServer>
#include <QTcpSocket>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrl>
#include <QDateTime>
#include <QTimer>
#include <QVariantMap>

#include "main.h"

#define MB_MAX_NETREQ_COUNT 10
#define MB_MAX_TIMER_COUNT 10

class CBaseControlPacket;
class CRoomConfigPacket;
class CRoomMessagePacket;
class CRoomPresencePacket;
class CScriptMessagePacket;
class CQuitPacket;

class CScriptRunnerBase : public QObject
{
	Q_OBJECT

protected:
	QString m_script;
	QString m_handle;
	QLocalSocket *m_comm;
	QByteArray m_sockData;
	QString m_roomJid;
	QString m_nickName;
	QString m_roomName;
	QString m_server;
	QStringList m_participants;
	bool m_allowListen;
	QVariantMap m_extraConfig;

	QNetworkAccessManager *m_netMan;

	int m_networkRequestCount;
	int m_timerCount;

private slots:
	void socketDisconnected();
	void socketReadyRead();

	void networkRequestFinished(QNetworkReply *reply);

	void timerTimeout();

	void handleNewTcpConnection();
	void handleTcpConnectionData();
	void handleTcpConnectionClose();

protected:
	virtual void onRoomConfigPacket(const CRoomConfigPacket &pkt);
	virtual void onRoomMessagePacket(const CRoomMessagePacket &pkt);
	virtual void onRoomPresencePacket(const CRoomPresencePacket &pkt);

	virtual void onNetworkRequestFinished(bool allOk, const QString &name, const QString &url, const QByteArray &data);

	virtual void onTimerTimeout(const QString &name);

	virtual bool onNewTcpConnection(const QTcpServer *srv, const QTcpSocket *peer)
	{
		Q_UNUSED(srv);
		Q_UNUSED(peer);
		return true;
	}

	virtual void onTcpConnectionData(const QTcpServer *srv, const QTcpSocket *client, const QByteArray &data)
	{
		Q_UNUSED(srv);
		Q_UNUSED(client);
		Q_UNUSED(data);
	}

	virtual void onTcpConnectionClosed(const QTcpServer *srv, const QTcpSocket *client)
	{
		Q_UNUSED(srv);
		Q_UNUSED(client);
	}

public:
	CScriptRunnerBase(const QString &handle, const QString &name, int fd, const QVariantMap &extraConfig,
	                  QObject *parent = 0);
	virtual ~CScriptRunnerBase();

	virtual bool setupScript() = 0;

	void sendMessage(const QString &to, const QString &body, const QString &subject = "", bool fixedFont = false);

	QString roomJid() const { return m_roomJid; }
	QString roomName() const { return m_roomName; }
	QString nickName() const { return m_nickName; }
	const QStringList &participants() const { return m_participants; }
	void networkRequest(const QString &name, const QString &url);
	void createTimer(const QString &name, int timeout);
	void localServerCreate(const QString &name, quint16 port, QTcpServer **server, QString &error);
	void localServerDestroy(const QString &name);
	void localServerDestroyClient(const QString &name, const QString &clientName);
	qint64 localServerSend(const QString &name, const QString &clientName, const char *data, qint64 size, QString &error);
	qint64 localClientSend(const QString &name, const char *data, qint64 size, QString &error);
	void localClientDestroy(const QString &name);

	void setInitialConfig(const QString &server, const QString &room, const QString &nickname)
	{
		m_server = server;
		m_roomName = room;
		m_nickName = nickname;
		m_roomJid = fmt("%1@%2").arg(room).arg(server);
	}

public slots:
	QString logHandle() { return m_handle; }
};

#endif // __CSCRIPTRUNNER_H_INCLUDED__
