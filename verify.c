#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>
#include <time.h>
#ifdef HAVE_SHADOW_H
 #include <shadow.h>
#endif


#include "definitions.h"
#include "limits.h"
// #include "log_message.h"
#include "md5crypt.h"
#include "environment.h"


#define __AUTHENTICATED__		0
#define __WRONG_PASSWORD__		1
#define __USER_EXPIRED__		2
#define __CANT_FETCH_SPWD__		3

char *crypt(const char *key, const char *salt);

static int CheckPassword(char *crypted, char *clear);
static int AuthNormal(struct passwd *passwd, char *clear);
static int AuthShadow(char * username, char *clear);
struct passwd *Verify(char *username, char *password, char *rmthost);

static int CheckPassword (char *crypted, char *clear)
{
#ifdef ALLOW_DES
	return (strcmp((char *)crypt(clear, crypted), crypted) == 0);
#else
 #ifdef ALLOW_MD5
	return (strcmp((char *)md5crypt(clear,crypted), crypted) == 0);
 #else
 #error "Either DES or MD5 must be allowed"
 #endif
#endif

#ifdef ALLOW_MD5
#ifdef ALLOW_DES

	if ((strcmp((char *)crypt(clear, crypted), crypted) != 0))
		return (strcmp((char *)md5crypt(clear, crypted),crypted) == 0);
	else
		return TRUE;

#endif
#endif
}

#ifdef ALLOW_NORMAL

static int AuthNormal(struct passwd *passwd, char *clear)
{
	if (CheckPassword(passwd->pw_passwd, clear))
		return __AUTHENTICATED__;
	else
		return __WRONG_PASSWORD__;
}

#endif

#ifdef HAVE_SHADOW_H

static int AuthShadow(char * username, char *clear)
{
	struct spwd *entry;
	time_t now;

	entry = getspnam(username);
 
	// entry will usually be NULL when the program runs with UID!=0.
	if (entry==NULL)
		return __CANT_FETCH_SPWD__;


	// what time is it?
	time(&now);
  
	// has the account expired?
	if ((entry->sp_expire != -1) && ((unsigned long)now/(60*60*24) > entry->sp_expire))
	{
		char errormessage[__MAX_STR_LEN__];

		sprintf(errormessage, "the account for %s has expired", username);
		// log_message(40000, errormessage);
		// log_message(49999, errormessage);
		return __USER_EXPIRED__;
	}

	if (CheckPassword(entry->sp_pwdp, clear))
		return __AUTHENTICATED__;
	else
		return __WRONG_PASSWORD__;
}

#endif

struct passwd *Verify(char *username, char *password, char *rmthost)
{
	struct passwd *passwd_entry;
	char errormessage [__MAX_STR_LEN__];

	// has the user entered a name?
	if (username[0]=='\0')
		return NULL;

	// is the user known to the system?
	if ((passwd_entry = getpwnam(username)) == NULL)
	{
		sprintf(errormessage, "%s is not known to the system!", username);
		// log_message (49998, errormessage);
		// log_message (49999, errormessage);
		return NULL;
    }

#ifdef SUPPORT_SECURETTY

	if ((passwd_entry->pw_uid==0) && !(root_allowed()))
	{
		sprintf(errormessage, "accorting to /etc/securetty root is not allowed to log here");
		// log_message (40000, errormessage); 
		// log_message (49999, errormessage); 
		return NULL;
	}

#endif

	if ((passwd_entry->pw_passwd[0])=='!')
	{
		sprintf(errormessage, "%s is locked", passwd_entry->pw_name);
		// log_message(40000+passwd_entry->pw_uid, errormessage);
		// log_message(49999, errormessage);
		return NULL;
	}

	if (passwd_entry->pw_passwd[0]=='\0')
	{
		sprintf (errormessage, "%s logged in", passwd_entry->pw_name);
		// log_message (30000+passwd_entry->pw_uid, errormessage);
		// log_message (39999, errormessage);
		return passwd_entry;
	}

	if (strcmp(passwd_entry->pw_passwd,"x")==0)  /* Do we use shadow? */
	{                                         

#ifdef HAVE_SHADOW_H

		switch (AuthShadow(username, password)) 
		{
			case __AUTHENTICATED__:
				sprintf(errormessage, "%s logged in", passwd_entry->pw_name);
				// log_message (30000+passwd_entry->pw_uid, errormessage);
				// log_message (39999, errormessage);
				return passwd_entry;
			case __USER_EXPIRED__:
				sprintf(errormessage, "%s expired", passwd_entry->pw_name);
				// log_message (40000+passwd_entry->pw_uid, errormessage);
				// log_message (49999, errormessage);
				break;
			case __CANT_FETCH_SPWD__:
				sprintf(errormessage,"couldn't fetch struct spwd for %s. Maybe not running as root?", passwd_entry->pw_name);
				// log_message(40000+passwd_entry->pw_uid, errormessage);
				// log_message(49999, errormessage);
		}

		return NULL;
#endif

	}
	else                                        
	{

#ifdef ALLOW_NORMAL

		if (AuthNormal(passwd_entry, password)==__AUTHENTICATED__)
		{
			sprintf(errormessage, "%s logged in", passwd_entry->pw_name);
			// log_message (30000+passwd_entry->pw_uid, errormessage);
			// log_message (39999, errormessage);
			return passwd_entry;
		}
		else
		{
			// Log that the authentication failed
			sprintf (errormessage, "%s entered a wrong password", passwd_entry->pw_name);
          		// log_message (40000+passwd_entry->pw_uid, errormessage);
          		// log_message (49999, errormessage);
			return NULL;
		}

#endif

	}

	return NULL;
}
