#ifndef __ENVIRONMENT_H__
#define __ENVIRONMENT_H__

#define FILENAME_LOGIN_LOGGING  "/etc/login.logging"

#define FILENAME_SECURETTY      "/etc/securetty"

#define FILENAME_LASTLOG        "/var/log/lastlog"

#define FILENAME_DEFAULTFLT     "/etc/default.flt"

#define MAILBOX_PATH 		"/var/spool/mail"

#define SUPPORT_SYSLOG

#define SUPPORT_FILELOG

// define ALLOW_NORMAL if you want to allow non-shadow password-verification.
#define ALLOW_NORMAL

// set CLOSE_AFTER_FAILED N if you want login to terminate
// after N failed logins.
#define CLOSE_AFTER_FAILED 5

// define ALLOW_NORMAL if you want to allow authenticating
// using DES-encryption. If you are unsure define it.
#define ALLOW_DES

// define ALLOW_MD5 if you want to allow authenticating
// using MD5-encryption. If you are unsure define it.
#define ALLOW_MD5

#define MSG_NEWMAIL "You've got mail!"
#define MSG_SEENMAIL "You've got old mail."
#define MSG_NOMAIL "No mails for you."

#define VERSION "0.09"

#define TITLE "VDI login "
#define LOGIN "Login name"
#define PASSWD "Password"
#define WELCOME "Welcome to "

#define BT_LOGIN "Login"
#define BT_REBOOT "Reboot"
#define BT_SHUTDOWN "Shutdown"


#endif
