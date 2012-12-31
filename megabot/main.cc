#include "main.h"
#include "cxmppserver.h"
#include "cxmpproom.h"
#include "cscriptrunner.h"

#include "cjsonparser.h"

#include <stdlib.h>
#include <stdio.h>
#include <getopt.h>

static void SIGINT_handler( int )
{
	if ( botInstance ) botInstance->quit();
}

static void print_usage()
{
	printf( "Usage:\n" );
	printf( "  megabot <options>\n" );
	printf( "\n" );
	printf( "Options:\n" );
	printf( "  -s, --start         - start the bot\n" );
	printf( "  -d, --no-daemon     - do NOT detach from the console\n" );
	printf( "  -k, --stop          - stop running bot instance\n");
	printf( "  -b <path>           - set the base path to 'path'\n" );
	printf( "    --basepath <path>\n" );
}

int main( int argc, char **argv )
{
	char *args[] = { argv[0], NULL };
	int   acnt   = 1;
	CMegaBot bot( acnt, args );

	struct option opts[] = {
		{ "start",      no_argument,       NULL, 's' },
		{ "stop",       no_argument,       NULL, 'k' },
		{ "no-daemon",  no_argument,       NULL, 'd' },
		{ "basepath",   required_argument, NULL, 'b' }
	};
	QString optstr = "+";
	bool start = false, stop = false, fork = true;
	QString basepath;
	int opt;

	for ( unsigned i = 0; i < sizeof( opts ) / sizeof( opts[0] ); i++ ) {
		if ( !opts[i].val ) continue;

		QString s( QChar( ( char )opts[i].val ) );
		switch ( opts[i].has_arg ) {
			case required_argument: {
				s += ":";
				break;
			}
			case optional_argument: {
				s += "::";
				break;
			}
			default: break;
		}

		optstr += s;
	}

	while ( ( opt = getopt_long( argc, argv, optstr.toUtf8().data(), opts, NULL ) ) != -1 ) {
		switch ( opt ) {
			case 's': {
				start = true;
				break;
			}
			case 'k': {
				stop = true;
				break;
			}
			case 'd': {
				fork = false;
				break;
			}
			case 'b': {
				basepath = QString( optarg );
				break;
			}
			default: {
				print_usage();
				return 1;
			}
		}
	}

	signal( SIGINT, SIGINT_handler );

	if ( QString( argv[0] ).startsWith( MB_SCRIPT_RUNNER_NAME ) ) {
		if ( !bot.initScriptRunner() ) return 1;
	} else {
		if ( !start && !stop ) {
			print_usage();
			return 1;
		}

		if ( stop ) {
			bot.triggerKillSwitch();
			return 0;
		}

		if ( !bot.initMaster( fork, basepath ) ) return 1;
	}

	return bot.exec();
}
