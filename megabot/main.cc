#include "main.h"
#include "cxmppserver.h"
#include "cxmpproom.h"
#include "cscriptrunner.h"

static void SIGINT_handler( int )
{
	if ( myApp ) {
		LOG( "Quiting" );
		myApp->quit();
	}
}

static void print_usage()
{
	QSettings setts;
	printf( "Usage:\n" );
	printf( "  megabot -D|-C|-w <path>\n" );
	printf( "\n" );
	printf( "Options:\n" );
	printf( "  -D        - detach from the console and run in the background (daemon)\n" );
	printf( "  -C        - do NOT detach from the console\n" );
	printf( "  -w <path> - write a dummy (empty) config file to the file specified by\n" );
	printf( "              'path'. The default config file name is:\n  %s\n", setts.fileName().toUtf8().data() );
}

int main( int argc, char **argv )
{
	QCoreApplication::setApplicationName( "MegaBot" );
	QCoreApplication::setOrganizationName( "[LtK]Studios" );
	QCoreApplication::setOrganizationDomain( "gargoylle.net" );

	signal( SIGINT, SIGINT_handler );

	char *args[] = { argv[0], NULL };
	int   acnt   = 1;
	CMegaBot a( acnt, args );

	if ( QString( argv[0] ).startsWith( MB_SCRIPT_RUNNER_NAME ) ) {
		if ( argc != 2 ) {
			LOG( "SCRIPT RUNNER: invalid usage" );
			return 1;
		}

		char *sz_fd     = getenv( "MEGABOT_CONTROL_SOCKET" );
		char *sz_server = getenv( "MEGABOT_SERVER" );
		char *sz_room   = getenv( "MEGABOT_ROOM" );
		char *sz_nick   = getenv( "MEGABOT_NICKNAME" );
		QString error   = "";

		if ( !sz_fd     ) error += " MEGABOT_CONTROL_SOCKET";
		if ( !sz_server ) error += " MEGABOT_SERVER";
		if ( !sz_room   ) error += " MEGABOT_ROOM";
		if ( !sz_nick   ) error += " MEGABOT_NICKNAME";
		if ( !error.isEmpty() ) {
			LOG( fmt( "SCRIPT RUNNER: missing the following env variable(s):%1" ).arg( error ) );
			return 2;
		}
		bool ok = true;
		int fd = QString( sz_fd ).toInt( &ok );
		if ( !ok || ( ok && fd < 0 ) ) {
			LOG( fmt( "SCRIPT RUNNER: invalid MEGABOT_CONTROL_SOCKET '%1'" ).arg( sz_fd ) );
			return 2;
		}

		QString qserver   = QString( sz_server ).trimmed();
		QString qroom     = QString( sz_room ).trimmed();
		QString qnickname = QString( sz_nick ).trimmed();

		if ( qserver.isEmpty() || qroom.isEmpty() || qnickname.isEmpty() ) {
			LOG( "SCRIPT RUNNER: invalid script params" );
			return 2;
		}

		if ( !a.initScriptRunner( argv[1], fd ) )
			return 1;

		a.scriptRunner()->setInitialConfig( qserver, qroom, qnickname );

		return a.exec();
	}

	if ( argc == 3 && !strcmp( argv[1], "-w" ) ) {
		a.writeDummyConfig( argv[2] );
		return 0;
	} else if ( argc == 2 && ( !strcmp( "-D", argv[1] ) || ( !strcmp( "-C", argv[1] ) ) ) ) {
		if ( !strcmp( "-D", argv[1] ) ) {
			if ( !a.initMaster( true ) ) return 1;
		} else {
			if ( !a.initMaster( false ) ) return 1;
		}
		LOG( fmt( "Entering main loop" ) );
		return a.exec();
	} else if ( argc == 2 && !strcmp( "-k", argv[1] ) ) {
		a.triggerKillSwitch();
		return 0;
	}

	print_usage();
	return 1;
}
