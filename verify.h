#ifndef __VERIFY_H__
#define __VERIFY_H__

#include <pwd.h>

struct passwd *Verify(char *username, char *password, char *rmthost);

#endif
