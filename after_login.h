#ifndef __HAVE_AFTER_LOGIN_H__
#define __HAVE_AFTER_LOGIN_H__

#include "config.h"

#include <sys/types.h>
#include <pwd.h>
#ifdef HAVE_LASTLOG_H
 #include <lastlog.h>
#endif

void AfterLogin(uid_t, char *, struct lastlog *);

#endif
