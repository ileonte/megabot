#include "ctlpackets.h"

#include <time.h>

static inline void pack_string(QByteArray &data, const QString &str)
{
	qint32 sl = str.toUtf8().size();
	data.append((const char *)&sl, 4);
	data.append(str.toUtf8(), sl);
}

static inline void pack_int(QByteArray &data, quint64 val)
{
	data.append((const char *)&val, 8);
}
static inline bool unpack_string(const QByteArray &data, qint32 &p, QString &result)
{
	if (data.size() - p < 4)
		return false;
	qint32 l = *((qint32 *)(data.data() + p));
	p += 4;
	if (p + l > data.size())
		return false;
	result = QString::fromUtf8(data.data() + p, l);
	p += l;
	return true;
}

static inline bool unpack_int(const QByteArray &data, qint32 &p, quint64 &result)
{
	if (data.size() - p < 8)
		return false;
	result = *((quint64 *)(data.data() + p));
	p += 8;
	return true;
}

void CBaseControlPacket::constructPacket(QByteArray &where, const QByteArray &data) const
{
	qint32 t = (qint32)m_type;
	qint32 l = (qint32)data.size() + 8;

	where.clear();
	where.append((const char *)&t, 4);
	where.append((const char *)&l, 4);
	where.append(data);
}

bool CBaseControlPacket::deconstructPacket(QByteArray &from, QByteArray &data) const
{
	if (from.size() < 8)
		return false;

	ControlPatcketType type = Unknown;
	qint32 l = 0;
	qint32 t = 0;

	t = *((qint32 *)(from.data()));
	l = *((qint32 *)(from.data() + 4));

	if (l > from.size())
		return false;

	data.clear();
	data.append(from.data() + 8, l - 8);
	from.remove(0, l);

	type = (ControlPatcketType)t;
	switch (type) {
		case RoomConfig:
		case RoomMessage:
		case RoomPresence:
		case ScriptMessage:
			break;
		default:
			return false;
	}

	if (type != m_type)
		return false;

	return true;
}

CRoomConfigPacket::CRoomConfigPacket(QObject *parent) : CBaseControlPacket(parent)
{
	m_type = RoomConfig;
}
CRoomConfigPacket::CRoomConfigPacket(CXMPPRoom *room, QObject *parent) : CBaseControlPacket(parent)
{
	m_type = RoomConfig;
	m_roomJid = room->bareJid();
	m_roomName = room->roomName();
	m_nickName = room->nickName();
	m_topic = room->topic();
}

void CRoomConfigPacket::pack(QByteArray &where) const
{
	QByteArray data;

	pack_string(data, m_roomJid);
	pack_string(data, m_roomName);
	pack_string(data, m_nickName);
	pack_string(data, m_topic);

	constructPacket(where, data);
}

bool CRoomConfigPacket::unpack(QByteArray &from)
{
	QByteArray data;
	qint32 p = 0;

	if (!deconstructPacket(from, data))
		return false;
	if (!unpack_string(data, p, m_roomJid))
		return false;
	if (!unpack_string(data, p, m_roomName))
		return false;
	if (!unpack_string(data, p, m_nickName))
		return false;
	if (!unpack_string(data, p, m_topic))
		return false;

	return true;
}

CRoomMessagePacket::CRoomMessagePacket(QObject *parent) : CBaseControlPacket(parent)
{
	m_type = RoomMessage;
}
CRoomMessagePacket::CRoomMessagePacket(const QXmppMessage &msg, QObject *parent) : CBaseControlPacket(parent)
{
	m_from = msg.from();
	m_body = msg.body();
	m_type = RoomMessage;
	m_serverTime = msg.stamp().toTime_t();
	m_localTime = time(NULL);
}

void CRoomMessagePacket::pack(QByteArray &where) const
{
	QByteArray data;

	pack_string(data, m_from);
	pack_string(data, m_body);
	pack_int(data, m_serverTime);
	pack_int(data, m_localTime);

	constructPacket(where, data);
}

bool CRoomMessagePacket::unpack(QByteArray &from)
{
	QByteArray data;
	qint32 p = 0;

	if (!deconstructPacket(from, data))
		return false;
	if (!unpack_string(data, p, m_from))
		return false;
	if (!unpack_string(data, p, m_body))
		return false;
	if (!unpack_int(data, p, m_serverTime))
		return false;
	if (!unpack_int(data, p, m_localTime))
		return false;

	return true;
}

CRoomPresencePacket::CRoomPresencePacket(QObject *parent) : CBaseControlPacket(parent)
{
	m_type = RoomPresence;
}
CRoomPresencePacket::CRoomPresencePacket(const QXmppPresence &presence, QObject *parent) : CBaseControlPacket(parent)
{
	m_type = RoomPresence;
	m_ptype = presence.type();
	m_who = presence.from();
	m_statusType = presence.availableStatusType();
	m_statusText = presence.statusText();
	m_statusPriority = presence.priority();
}

void CRoomPresencePacket::pack(QByteArray &where) const
{
	QByteArray data;
	qint32 pt = (qint32)m_ptype;
	qint32 st = (qint32)m_statusType;

	pack_int(data, pt);
	pack_string(data, m_who);
	pack_int(data, st);
	pack_string(data, m_statusText);
	pack_int(data, m_statusPriority);

	constructPacket(where, data);
}

bool CRoomPresencePacket::unpack(QByteArray &from)
{
	QByteArray data;
	qint32 p = 0;
	quint64 t = 0;

	if (!deconstructPacket(from, data))
		return false;
	if (!unpack_int(data, p, t))
		return false;
	switch ((QXmppPresence::Type)t) {
		case QXmppPresence::Error:
		case QXmppPresence::Available:
		case QXmppPresence::Unavailable:
		case QXmppPresence::Subscribe:
		case QXmppPresence::Subscribed:
		case QXmppPresence::Unsubscribe:
		case QXmppPresence::Unsubscribed:
		case QXmppPresence::Probe:
			break;
		default:
			return false;
	}
	m_ptype = (QXmppPresence::Type)t;
	if (!unpack_string(data, p, m_who))
		return false;
	if (!unpack_int(data, p, t))
		return false;
	switch ((QXmppPresence::AvailableStatusType)t) {
		case QXmppPresence::Online:
		case QXmppPresence::Away:
		case QXmppPresence::XA:
		case QXmppPresence::DND:
		case QXmppPresence::Chat:
		case QXmppPresence::Invisible:
			break;
		default:
			return false;
	}
	m_statusType = (QXmppPresence::AvailableStatusType)t;
	if (!unpack_string(data, p, m_statusText))
		return false;
	if (!unpack_int(data, p, t))
		return false;
	m_statusPriority = (int)t;

	return true;
}

CScriptMessagePacket::CScriptMessagePacket(QObject *parent) : CBaseControlPacket(parent)
{
	m_type = ScriptMessage;
}
CScriptMessagePacket::CScriptMessagePacket(const QString &to, const QString &body, const QString &subject, bool fixedFont,
                                           QObject *parent)
    : CBaseControlPacket(parent)
{
	m_type = ScriptMessage;
	m_to = to;
	m_body = body;
	m_subject = subject;
	m_fixedFont = fixedFont;
}

void CScriptMessagePacket::pack(QByteArray &where) const
{
	QByteArray data;

	pack_string(data, m_to);
	pack_string(data, m_body);
	pack_string(data, m_subject);
	pack_int(data, m_fixedFont);

	constructPacket(where, data);
}

bool CScriptMessagePacket::unpack(QByteArray &from)
{
	QByteArray data;
	qint32 p = 0;

	if (!deconstructPacket(from, data))
		return false;
	if (!unpack_string(data, p, m_to))
		return false;
	if (!unpack_string(data, p, m_body))
		return false;
	if (!unpack_string(data, p, m_subject))
		return false;
	if (!unpack_int(data, p, m_fixedFont))
		return false;

	return true;
}
