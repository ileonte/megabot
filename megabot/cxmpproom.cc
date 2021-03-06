#include "cxmpproom.h"
#include "cscriptcontroller.h"
#include "ctlpackets.h"

#include <QXmppUtils.h>

CXMPPRoom::CXMPPRoom(CXMPPServer *parent) : QObject(parent)
{
	m_server = parent;
	m_mucman = parent->m_mucman;
	m_mucroom = NULL;
}

CXMPPRoom::~CXMPPRoom()
{
	part();
}
void CXMPPRoom::topicChanged()
{
	handleRoomConfig();
}
CScriptController *CXMPPRoom::findScript(const QString &name)
{
	for (int i = 0; i < m_scripts.size(); i++)
		if (m_scripts[i]->script() == name)
			return m_scripts[i];
	return NULL;
}

void CXMPPRoom::roomJoined()
{
	for (int i = 0; i < m_scripts.size(); i++) {
		CScriptController *script = m_scripts[i];
		if (script->autoRun())
			script->runScript();
	}

	connect(m_mucroom, SIGNAL(subjectChanged(QString)), this, SLOT(topicChanged()));
}

QString CXMPPRoom::topic() const
{
	if (!m_mucroom)
		return "";

	return m_mucroom->subject();
}

bool CXMPPRoom::join()
{
	if (isEmpty())
		return false;

	if (!m_mucroom) {
		m_mucroom = m_mucman->addRoom(bareJid());
		connect(m_mucroom, SIGNAL(joined()), this, SLOT(roomJoined()));
	}

	m_mucroom->setNickName(m_nickName);
	return m_mucroom->join();
}

void CXMPPRoom::part()
{
	if (!m_mucroom)
		return;

	for (int i = 0; i < m_scripts.size(); i++)
		m_scripts[i]->stopScript();

	m_mucroom->leave();
	delete m_mucroom;
	m_mucroom = NULL;
}

void CXMPPRoom::handleRoomConfig()
{
	for (int i = 0; i < m_scripts.size(); i++) {
		CScriptController *script = m_scripts[i];
		script->sendRoomConfig();
	}
}

void CXMPPRoom::handleRoomPresence(const QXmppPresence &presence)
{
	QString nickName = QXmppUtils::jidToResource(presence.from());
	switch (presence.type()) {
		case QXmppPresence::Available: {
			m_participants.append(nickName);
			break;
		}
		case QXmppPresence::Unavailable: {
			if (nickName != m_nickName) {
				m_participants.removeAll(nickName);
			} else {
				part();
				return;
			}
			break;
		}
		default: {
			break;
		}
	}

	for (int i = 0; i < m_scripts.size(); i++)
		m_scripts[i]->sendRoomPresence(presence);
}

void CXMPPRoom::handleRoomMessage(const QXmppMessage &msg)
{
	if (QXmppUtils::jidToResource(msg.from()) == "")
		return;

	for (int i = 0; i < m_scripts.size(); i++)
		m_scripts[i]->sendRoomMessage(msg);
}

void CXMPPRoom::addScript(CScriptController *script)
{
	if (!m_scripts.contains(script)) {
		connect(script, SIGNAL(message(CScriptMessagePacket)), this, SLOT(scriptMessage(CScriptMessagePacket)));
		m_scripts.append(script);
	}
}

bool CXMPPRoom::runScript(const QString &name)
{
	CScriptController *script = findScript(name);
	if (!script) {
		script = new CScriptController(this, name);
		addScript(script);
	}

	if (!script->running())
		return script->runScript();

	return true;
}

void CXMPPRoom::scriptMessage(const CScriptMessagePacket &pkt)
{
	if (pkt.to() == bareJid()) {
		QXmppMessage msg;

		msg.setTo(pkt.to());
		msg.setType(QXmppMessage::GroupChat);
		msg.setBody(pkt.body());
		if (pkt.fixedFont()) {
			QString str = pkt.body();

			str.replace('&', "&amp;");
			str.replace('<', "&lt;");
			str.replace('>', "&gt;");
			str.replace('\'', "&apos;");
			str.replace('"', "&quot;");
			str.replace('\n', "<br/>");

			msg.setXhtml(QString("<html xmlns='http://jabber.org/protocol/xhtml-im'>"
			                     "<body xmlns='http://www.w3.org/1999/xhtml'>"
			                     "<span style='font-family: DejaVu Sans Mono, Courier "
			                     "New;'>%1</span></body></html>")
			                 .arg(str));
		}

		m_server->m_client->sendPacket(msg);
	} else {
		m_server->m_client->sendMessage(pkt.to(), pkt.body());
	}
}
