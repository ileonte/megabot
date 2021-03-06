#ifndef __CTLPACKETS_H_INCLUDED__
#define __CTLPACKETS_H_INCLUDED__

#include <QObject>
#include <QByteArray>

#include "cxmpproom.h"

class CBaseControlPacket : public QObject
{
	Q_OBJECT

public:
	enum ControlPatcketType { Unknown, RoomConfig, RoomMessage, RoomPresence, ScriptMessage };

	CBaseControlPacket(QObject *parent = 0) : QObject(parent) {}
	virtual ~CBaseControlPacket() {}
	ControlPatcketType type() const { return m_type; }
	virtual void pack(QByteArray &where) const = 0;
	virtual bool unpack(QByteArray &from) = 0;

protected:
	ControlPatcketType m_type;

	void constructPacket(QByteArray &where, const QByteArray &data) const;
	bool deconstructPacket(QByteArray &from, QByteArray &data) const;
};

class CRoomConfigPacket : public CBaseControlPacket
{
	Q_OBJECT

private:
	QString m_roomJid;
	QString m_roomName;
	QString m_nickName;
	QString m_topic;

public:
	CRoomConfigPacket(QObject *parent = 0);
	CRoomConfigPacket(CXMPPRoom *room, QObject *parent = 0);
	virtual ~CRoomConfigPacket() {}
	QString roomJid() const { return m_roomJid; }
	QString roomName() const { return m_roomName; }
	QString nickName() const { return m_nickName; }
	QString topic() const { return m_topic; }
	void pack(QByteArray &where) const;
	bool unpack(QByteArray &from);
};

class CRoomMessagePacket : public CBaseControlPacket
{
	Q_OBJECT

private:
	QString m_from;
	QString m_body;
	quint64 m_serverTime;
	quint64 m_localTime;

public:
	CRoomMessagePacket(QObject *parent = 0);
	CRoomMessagePacket(const QXmppMessage &msg, QObject *parent = 0);
	virtual ~CRoomMessagePacket() {}
	QString from() const { return m_from; }
	QString body() const { return m_body; }
	quint64 serverTime() const { return m_serverTime; }
	quint64 localTime() const { return m_localTime; }
	void pack(QByteArray &where) const;
	bool unpack(QByteArray &from);
};

class CRoomPresencePacket : public CBaseControlPacket
{
	Q_OBJECT

private:
	QXmppPresence::Type m_ptype;
	QString m_who;
	QXmppPresence::AvailableStatusType m_statusType;
	QString m_statusText;
	int m_statusPriority;

public:
	CRoomPresencePacket(QObject *parent = 0);
	CRoomPresencePacket(const QXmppPresence &presence, QObject *parent = 0);
	virtual ~CRoomPresencePacket() {}
	QXmppPresence::Type presenceType() const { return m_ptype; }
	QString who() const { return m_who; }
	QXmppPresence::AvailableStatusType statusType() const { return m_statusType; }
	QString statusText() const { return m_statusText; }
	int statusPriority() const { return m_statusPriority; }
	void pack(QByteArray &where) const;
	bool unpack(QByteArray &from);
};

class CScriptMessagePacket : public CBaseControlPacket
{
	Q_OBJECT

private:
	QString m_to;
	QString m_body;
	QString m_subject;
	quint64 m_fixedFont;

public:
	CScriptMessagePacket(QObject *parent = 0);
	CScriptMessagePacket(const QString &to, const QString &body, const QString &subject = "", bool fixedFont = false,
	                     QObject *parent = 0);
	virtual ~CScriptMessagePacket() {}
	QString to() const { return m_to; }
	QString body() const { return m_body; }
	QString subject() const { return m_subject; }
	bool fixedFont() const { return m_fixedFont ? true : false; }
	void pack(QByteArray &where) const;
	bool unpack(QByteArray &from);
};

#endif // __CTLPACKETS_H_INCLUDED__
