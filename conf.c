#include "config.h"
#include <string.h>
#include <stdlib.h>
#include "conf.h"

static int CONFSupportSecuretty = 1;
static int CONFSetPathEnv = 1;
static int CONFCheckEmail = 1;

// 0 means "no timeout"
static int CONFTimeout = 0;

int GetTimeout(void)
{
	return CONFTimeout;
}

void SetTimeout(int val)
{
	CONFTimeout = val;
}

int GetSetPathEnv(void)
{
	return CONFSetPathEnv;
}

void SetSetPathEnv(int val)
{
	CONFSetPathEnv = val;
}

int GetCheckEmail(void)
{
	return CONFCheckEmail;
}

void SetCheckEmail(int val)
{
	CONFCheckEmail = val;
}

int GetSupportSecuretty(void)
{
	return CONFSupportSecuretty;
}

void SetSupportSecuretty(int val)
{
	CONFSupportSecuretty = val;
}
