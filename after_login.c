#include "config.h"
#include "conf.h"
#include "after_login.h"
#include "limits.h"
#include "definitions.h"
#include "environment.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pwd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <utmp.h>
#include <time.h>
#include <malloc.h>

static void PrintWelcome(struct passwd *userinfo);
static int PrintLastlog(struct lastlog *oldlog);
static void PrintMotd(void);
static void CheckMail(struct passwd *userinfo);
void AfterLogin(uid_t uid, char *rmthost, struct lastlog *oldlog);

static void PrintWelcome(struct passwd *userinfo)
{
	char *name;

	if ((userinfo->pw_gecos == NULL) || (userinfo->pw_gecos[0] == 0))
		name=userinfo->pw_name;
	else
		name=strtok(userinfo->pw_gecos,",");

	printf("Welcome, %s!\n\n", name);
}  

static int PrintLastlog(struct lastlog *oldlog)
{
	char *strtime;

	// convert ll_time to a human-readable string
	strtime = ctime(&(oldlog->ll_time));

	// print some information
	printf(oldlog->ll_time?"Last login was from %s, at %s\n": "This is your first login\n", oldlog->ll_line, strtime);

	return 0;
}

static void PrintMotd(void)
{
	FILE *motd;
	char c;

	if ((motd = fopen("/etc/motd", "r")) != NULL)
	{
		while ((c = fgetc(motd)) != EOF)
			fputc(c, stdout);

	fclose(motd);
	}
}

static void CheckMail(struct passwd *userinfo)
{
	struct stat statbuf;
	char *mailbox;  

	if (GetCheckEmail())
	{
		if ((mailbox = getenv("MAILDIR")) != NULL) 
		{
			char *newmail = malloc(strlen(mailbox) + 5);

			if (newmail)
			{
				// What's the mailbox filename?
				sprintf(newmail, "%s/new", mailbox);
    
				// Now let's get some information on the file
				if ((stat(newmail, &statbuf) != -1) && (statbuf.st_size != 0))
					// is st_mtime (time of last modification) greater than
					// st_atime (time of last access)?
					if (statbuf.st_mtime > statbuf.st_atime)
					{
						free(newmail);
						printf(MSG_NEWMAIL);
						return;
					}
  
			free(newmail);
			}
		}
    
		mailbox = malloc(strlen(MAILBOX_PATH)+strlen(userinfo->pw_name)+1);

		if (mailbox)
		{
			strcpy(mailbox,MAILBOX_PATH);
			strcat(mailbox,userinfo->pw_name);

			if ((stat(mailbox, &statbuf) == -1) || (statbuf.st_size == 0))
				printf(MSG_NOMAIL);
			else 
 
			if (statbuf.st_atime > statbuf.st_mtime)
				printf(MSG_SEENMAIL);
			else
				printf(MSG_NEWMAIL);
		}
	
		printf("\n\n");
	}
}
   
void AfterLogin(uid_t uid, char *rmthost, struct lastlog *oldlog)
{
	struct passwd *userinfo;

	printf("\033E");

	userinfo = getpwuid(uid);
  
	if (userinfo) 
		PrintWelcome(userinfo);
    
	PrintLastlog(oldlog);
	PrintMotd();
	printf("\n");

	if (userinfo)
		CheckMail(userinfo);

	fflush(stdout);
}
