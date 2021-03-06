#include <QObject>
#include <QMetaMethod>
#include <QDateTime>

#include "main.h"

#include "cxmpproom.h"
#include "cxmppserver.h"
#include "cscriptrunner.h"
#include "cscriptcontroller.h"

#include "cluarunner.h"

static inline QString __t()
{
	return QDateTime::currentDateTime().toString("yyyy MMM dd hh:mm:ss");
}
void mb_trace(QObject *obj, const QString &message)
{
	QString handle;
	CXMPPServer *srv;
	CXMPPRoom *room;
	CScriptController *sctl;
	CScriptRunnerBase *srun;
	CMegaBot *bot;
	QString channelName;

	if ((bot = qobject_cast<CMegaBot *>(obj)) != NULL) {
		channelName = "system";
		handle = bot->logHandle();
	} else if ((srv = qobject_cast<CXMPPServer *>(obj)) != NULL) {
		channelName = "server";
		handle = srv->logHandle();
	} else if ((room = qobject_cast<CXMPPRoom *>(obj)) != NULL) {
		channelName = "room";
		handle = room->logHandle();
	} else if ((sctl = qobject_cast<CScriptController *>(obj)) != NULL) {
		channelName = "master";
		handle = sctl->logHandle();
	} else if ((srun = qobject_cast<CScriptRunnerBase *>(obj)) != NULL) {
		channelName = "script";
		handle = srun->logHandle();
	} else {
		channelName = "system";
		handle = fmt("NOHANDLE( %1 )").arg(obj->metaObject()->className());
	}

	QString msg = fmt("%1 %2 %3 %4 : %5").arg(__t()).arg(getpid(), 5).arg(channelName, 6).arg(handle).arg(message);
	if (botInstance->forked()) {
		msg += "\n";
		FILE *f = fopen(fmt("%1/megabot.log").arg(botInstance->logPath()).toUtf8().data(), "a+");
		if (f) {
			fprintf(f, "%s", msg.toUtf8().data());
			fclose(f);
		}
	} else {
		printf("%s\n", msg.toUtf8().data());
	}
}
