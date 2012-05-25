#ifndef __MAIN_H_INCLUDED__
#define __MAIN_H_INCLUDED__

#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <unistd.h>

#include "cmegabot.h"

#define fmt( __s ) QString( __s )
#define LOG( __s ) ({ \
	if ( myApp ) { \
		myApp->log( __LINE__, __FILE__, QString( __s ) ); \
	} else { \
		printf( "%s\n", QString( __s ).toUtf8().data() ); \
	} \
	0; \
})
#define ptr( __p ) ({ char __b[32] = {}; snprintf( __b, sizeof( __b ), "%p", ( void * )__p ); QString __r( __b ); __r; })

#endif // __MAIN_H_INCLUDED__
