#ifndef __VDI_LOGIN_H__
#define __VDI_LOGIN_H__

#include "list.h"


typedef struct
{
	char *menuItem;
	char *command;
	char **childargv;
} sCommand; 

sCommand *shell;

#endif
