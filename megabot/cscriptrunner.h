#ifndef __CSCRIPTRUNNER_H_INCLUDED__
#define __CSCRIPTRUNNER_H_INCLUDED__

#include <QObject>
#include <QStringList>
#include <QLocalSocket>
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
	QVariantMap m_extraConfig;

	QNetworkAccessManager *m_netMan;

	int m_networkRequestCount;
	int m_timerCount;

private slots:
	void socketDisconnected();
	void socketReadyRead();

	void networkRequestFinished(QNetworkReply *reply);

	void timerTimeout();

protected:
	virtual void onRoomConfigPacket(const CRoomConfigPacket &pkt);
	virtual void onRoomMessagePacket(const CRoomMessagePacket &pkt);
	virtual void onRoomPresencePacket(const CRoomPresencePacket &pkt);

	virtual void onNetworkRequestFinished(bool allOk, const QString &name, const QString &url, const QByteArray &data);

	virtual void onTimerTimeout(const QString &name);

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
