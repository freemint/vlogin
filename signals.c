/*
#include "log_message.h"
#include "emergency.h"
*/
#include <signal.h>

static void HandleSigint(int val);
static void HandleSigterm(int val);
static void HandleMisc(int val);

void InstallHandlers();
  
void HandleSigint(int val)
{
	exit(-1);
}

void HandleSigterm(int val)
{
	exit(0);
}

void HandleMisc(int val)
{
/*  log_message (1, "OOPS! I recieved an error-signal, please report this error, \
                   sending us the core-dump, your configuration files, \
                   and some information on your system.");
*/
	exit(-1);
}

void InstallHandlers()
{
	signal(SIGINT, HandleSigint);
	signal(SIGILL, HandleMisc);
	signal(SIGTERM, HandleSigterm);
	signal(SIGFPE, HandleMisc);
}