#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pwd.h>
#include <grp.h>
#include <utmp.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <time.h>
#include <string.h>

#include <mint/ssystem.h>
#include <mint/mintbind.h>

#include "config.h"
#include "environment.h"

#include "limits.h"
#include "vdi_login.h"
// #include "log_message.h"
// #include "emergency.h"
#include "after_login.h"
#include "conf.h"

#define _WTMP_FILE "/var/log/wtmp"


static char **GetNewEnv (struct passwd *user);
static int WriteLastLogEntry(uid_t user_uid, char *rmthost, struct lastlog *oldlog);
int Logon(struct passwd *user, char *rmthost, int preserve);


char * emergency_env[] = {"PATH=/bin:/usr/bin:", "SHELL=/bin/sh", NULL};

struct utmp utent;

void CheckUtmp(int picky)
{
	char *line;
	struct utmp *ut;
	pid_t pid = getpid();

	setutent();

	// First, try to find a valid utmp entry for this process.
	while ((ut = getutent()))
		if (ut->ut_pid == pid && ut->ut_line[0] && ut->ut_id[0] &&
			(ut->ut_type == LOGIN_PROCESS || ut->ut_type == USER_PROCESS))
			break;

	// If there is one, just use it, otherwise create a new one.
	if (ut)
	{
		utent = *ut;
	}
	else
	{
		line = ttyname(0);

		if (!line)
		{
			exit(1);
		}

		if (strncmp(line, "/dev/", 5) == 0)
			line += 5;
			
		memset((void *) &utent, 0, sizeof utent);
		utent.ut_type = LOGIN_PROCESS;
		utent.ut_pid = pid;
		strncpy(utent.ut_line, line, sizeof utent.ut_line);

		// XXX - assumes /dev/tty??
		strncpy(utent.ut_id, utent.ut_line + 3, sizeof utent.ut_id);
		strcpy(utent.ut_user, "LOGIN");
		utent.ut_time = time(NULL);
	}
}

void SetUtmp(const char *name, const char *line, const char *host)
{
	utent.ut_type = USER_PROCESS;
	strncpy(utent.ut_user, name, sizeof utent.ut_user);
	utent.ut_time = time(NULL);

	// other fields already filled in by checkutmp above
	setutent();
	pututline(&utent);
	endutent();

#if HAVE_UPDWTMP
	updwtmp(_WTMP_FILE, &utent);
#endif
}

static char **GetNewEnv (struct passwd *user)
{
  #define __NUM_VARIABLES__ 7 

	int i;
	char **ne;

	ne = (char **)malloc(sizeof(char *) * (__NUM_VARIABLES__));
	if (ne == NULL)
		return ne;

	for (i = 0;i < __NUM_VARIABLES__;i++)
	{
		ne[i] = (char *)malloc(sizeof(char) * (__MAX_STR_LEN__));
		
		if (ne[i] == NULL)
		{
			return NULL;
		}
	}

	i = 0;

	sprintf(ne[i++], "HOME=%s", user->pw_dir);
	sprintf(ne[i++], "SHELL=%s", user->pw_shell);
	sprintf(ne[i++], "USER=%s", user->pw_name);
	sprintf(ne[i++], "LOGNAME=%s", user->pw_name);
	sprintf(ne[i++], "TERM=%s", getenv("TERM") ? getenv("TERM") : "vt52");

	if (GetSetPathEnv())
	{
		if ((user->pw_uid) == 0)
			sprintf(ne[i++], "PATH=/sbin:/bin:/usr/sbin:/usr/bin");
		else
			sprintf(ne[i++], "PATH=/usr/local/bin:/bin:/usr/bin:.");
	}

	ne[i] = NULL;

	return ne;
}

static int WriteLastLogEntry(uid_t user_uid, char *rmthost, struct lastlog *oldlog)
{
	// structure for parsing lastlog
	struct lastlog lastlog;
	// file-handler for /var/log/lastlog
	int lastlogFile;

	// open lastlog for writing
	if ((lastlogFile = open (FILENAME_LASTLOG, O_RDWR)) == -1)
	{
		// log_message (10001, "couldn't open FILENAME_LASTLOG for O_RDWR!");
		return -1;
	}

	// what is our line?
	sscanf(ttyname(0), "/dev/%s", lastlog.ll_line);

	// what host do we log on from?
	strcpy(lastlog.ll_host, rmthost);

	// what is the time?
	time(&lastlog.ll_time);

	// what user logged on?
	lseek(lastlogFile, (unsigned long)user_uid, SEEK_SET);

	// read old data
	read(lastlogFile, oldlog, sizeof *oldlog);

	// "reseek" to where we just read from
	lseek (lastlogFile, (unsigned long)user_uid, SEEK_SET);

	// write data
	write (lastlogFile, (char *)&lastlog, sizeof lastlog);

	// close file
	close (lastlogFile);

	return 0;
}

int Logon(struct passwd *user, char *rmthost, int preserve)
{
	char **newEnvironment;
	struct group *grp;
	struct lastlog oldlog;
	char * line;
	int i = 0;

	CheckUtmp(strcmp(user->pw_name,"root"));

	// Update utmp and wtmp entries
	line = malloc(strlen(ttyname(0)) - 5 + 1);

	if (line != NULL)
	{
		sscanf(ttyname(0),"/dev/%s",line);
		SetUtmp(user->pw_name,line,rmthost?rmthost:"");
		free(line);
	}

	// get user's environment, if it fail, 
	// use the emergency environment.
	if ((newEnvironment = GetNewEnv(user)) == NULL)
		newEnvironment = emergency_env;
   
	// cd to user's home
	chdir(user->pw_dir);

	// change permissions to terminal
	grp = getgrnam("tty");
	chown(ttyname(0), user->pw_uid, grp->gr_gid);
	chmod(ttyname(0), 0620);

	WriteLastLogEntry(user->pw_uid, rmthost?rmthost:"", &oldlog);

	// change to user's uid and gid
	setgid(user->pw_gid);
	initgroups(user->pw_name, user->pw_gid);
	setuid(user->pw_uid);
  
	AfterLogin(user->pw_uid, rmthost ? rmthost :"", &oldlog);

	while (newEnvironment[i] != NULL)
		putenv(newEnvironment[i++]);

	// execute user's shell
	execl(shell->command, shell->command, NULL);

	return 0;
}
