#include <mintbind.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <utmp.h>
#include <sys/stat.h>
#include <malloc.h>
#include <string.h>
//---------------------
#include "vdi_login.h"
#include "environment.h"
#include "signals.h"
#include "verify.h"
#include "logon.h"
#include "limits.h"
//---------------------
#include "tiny_aes.h"
#include "list.h"
#include "debug.h"


const char kSpace  = ' ';
const char kTab    = '\t';
const char kCommas = '"';


// #define AES_RUN

#ifdef AES_RUN
 sInfo info;
 sRect rect;
 sRect dest;
#endif

char username[__LEN_USERNAME__ + 1] = "";
char password[__LEN_PASSWORD__ + 1] = "";

extern int EventLoop();

#ifdef AES_RUN
 extern void InitColors();
 extern void DrawDesktop();
#endif

void *BuildLoginDialog(void *menuPtr);
void HandleLoginDialog(void *dialogPtr, void *menuPtr);

void *BuildMenu();

static char *CleanLine(char *buffer);
sList *ReadConfig();

void BuildWelcomeString(char *string);

void SystemReset();


sList *list = NULL;


#ifdef AES_RUN
void FakeInitTinyAES()
{
	handle = info.handle;

	vdi_inf[0] = info.screen_size.w;
	vdi_inf[1] = info.screen_size.h;
	vdi_inf[2] = info.sc_bpp;

	InitColors();
	
	DrawDesktop();
}
#endif

int main()
{
	void *dialogPtr;
	void *menuPtr;

 	#ifdef DEBUG
	 InitDebug();
	#endif
	#ifndef AES_RUN
	  DEBUG("Starting init: ");
	 InitTinyAES();
	  DEBUG("Ok\n");
	#endif
	#ifdef AES_RUN
	 info = FSA_Init(1);

	 dest.w = rect.w = 640;
	 dest.h = rect.h = 480;

 	 FSA_CreatWindow(rect);
	 dest = FSA_CenterImg(rect);
	 FakeInitTinyAES();
	#endif
	
	 DEBUG("Creating menu\n");
	menuPtr = BuildMenu();

	 DEBUG("Creating login dialog\n");
	dialogPtr = BuildLoginDialog(menuPtr);
	HandleLoginDialog(dialogPtr, menuPtr);

	 DEBUG("Exiting TAES");
	ExitTinyAES();

	#ifdef DEBUG
	 ExitDebug();
	#endif
	return -1;
}

void *BuildMenu()
{
	sElement *element;
	sCommands *commands;
	void *menuPtr = CreateMenu();
	int i;
	
	 DEBUG("Reading config: ");
	list = ReadConfig();
	 DEBUG("Ok\n");
	
	if (!list)
	{
		sCommands *command;
		char menuItem[] = "/bin/sh";

		 DEBUG("Creating default menu: ");

		list = CreateList();

		command = malloc(sizeof(sCommands));
				
		command->menuItem = malloc(sizeof(menuItem));
		strcpy(command->menuItem, menuItem);

		command->command = malloc(sizeof(menuItem));
		strcpy(command->command, menuItem);

		// set up defaults
		PushBack(list, command);
		 DEBUG("Ok\n");

		 DEBUG("Invalid vlogin.conf dialog: ");
		// if there is no valid command in vlogin.conf
		InfoDialog("vlogin.conf is invalid!", "using '/bin/sh'", "Ops");
		 DEBUG("Ok\n");
	}

	 DEBUG("Adding menu items:\n");
	element = list->first;
	shell = commands = element->data;

	for(i = 0; i < list->itemCount; i++)
	{
		 DEBUG(" item: %s\ncommand: %s\n", commands->menuItem, commands->command);
		AttachMenuItem(menuPtr, (void *)commands->menuItem);

		element = (void *)element->next;
		commands = element != 0 ? element->data : 0;
	}
	 DEBUG("Done\n");

	return menuPtr;
}

void *BuildLoginDialog(void *menuPtr)
{
	void *dialogPtr;
	char string[30] 	= WELCOME;
	
	sRect window		= {vdi_inf[0] / 2 - 160, vdi_inf[1] / 2 - 86, vdi_inf[0] / 2 + 160, vdi_inf[1] / 2 + 86};
	sRect button0		= {16, 136, 92, 158};
	sRect button1		= {256, 137, 276, 157};
	sRect button2		= {284, 137, 304, 157};
	sRect box			= {16, 60, 304, 125};
	sRect editField0	= {112, 69, 296, 89};
	sRect editField1	= {112, 95, 296, 115};
	sRect string0		= {16, 46, 0, 0};
	sRect string1		= {24, 84, 0, 0};
	sRect string2		= {24, 110, 0, 0};
	sRect comboBox		= {98, 137, 244, 157};

	 DEBUG("Creating dialog: ");
	dialogPtr = CreateDialog(window, TITLE);
	 DEBUG("Ok\n");

//	printf("create dialog\n");

	 DEBUG("BuildWelcomeString\n");
	BuildWelcomeString(string);
	 DEBUG("Done\n");

	 DEBUG("Attaching dialog items: ");
	AttachBox(dialogPtr, box, NULL);
	AttachString(dialogPtr, string0, string);
	AttachString(dialogPtr, string1, LOGIN);
	AttachString(dialogPtr, string2, PASSWD);
	AttachEditField(dialogPtr, editField0, FIELD_NORMAL, __LEN_USERNAME__, username);
	AttachEditField(dialogPtr, editField1, FIELD_MASKED, __LEN_PASSWORD__, password);
	AttachButton(dialogPtr, button0, BUTTON_CENTER + BUTTON_DEFAULT, "Login to");
	AttachButton(dialogPtr, button1, BUTTON_CENTER, "R");
	AttachButton(dialogPtr, button2, BUTTON_CENTER, "S");
	AttachComboBox(dialogPtr, comboBox, menuPtr);
	 DEBUG("Ok\n");

	 DEBUG("DrawDialog: ");
	DrawDialog(dialogPtr);
	 DEBUG("Ok\n");

	return dialogPtr;
}

void HandleLoginDialog(void *dialogPtr, void *menuPtr)
{
	sElement *element;

	struct passwd *user = NULL;
	char *rmthost = NULL;
	int i;

	if (!list) return;

	 DEBUG("StartingEventLoop\n");

	switch(EventLoop())
	{
		case 8:
			element = list->first;

			// get selected menu item id
			for(i = 0; i < GetMenuSelect(menuPtr); i++)
				element = (void *)element->next;

			shell = element != 0 ? element->data : shell;

			 DEBUG("Command: %s\n", shell->command);
			 DEBUG("ComboBox value: %d\n", GetMenuSelect(menuPtr));

			InstallHandlers();

			 DEBUG("Verify\n");

			if ((user = Verify(username, password, rmthost)) != NULL)
			{
				int i;
	
			 DEBUG("Verify OK\n");
				for (i = 0; i < sizeof(password); i++)
					password[i]=rand()%256;
			}
			else
			{
			 DEBUG("Verify FAIL\n");
				InfoDialog("User name or password you", "typed was incorrect!", "Ops");
		
			 DEBUG("Verify INFO\n");
				password[0] = 0;
		
				RedrawElement(dialogPtr, 5);
				ActivateDialog(dialogPtr);
		
				HandleLoginDialog(dialogPtr, menuPtr);
			}

 			#ifdef DEBUG
			 ExitDebug();
			#endif

			DisposeDialog(dialogPtr);
			ExitTinyAES();

			if (user)
			{
				Logon(user, rmthost, -1);
				SystemReset();
			}

			exit(0);
			break;

		case 9:
			if (AlertDialog("Do you realy want to", "REBOOT?", "Yes", "No"))
			{
	 			#ifdef DEBUG
				 ExitDebug();
				#endif

				DisposeDialog(dialogPtr);
				ExitTinyAES();

				printf("Rebooting system...");

				Shutdown(1);
			}
		
			ActivateDialog(dialogPtr);
		
			HandleLoginDialog(dialogPtr, menuPtr);
			break;

		case 10:
			if (AlertDialog("Do you realy want to", "SHUTDOWN?", "Yes", "No"))
			{
	 			#ifdef DEBUG
				 ExitDebug();
				#endif

				DisposeDialog(dialogPtr);
				ExitTinyAES();

				printf("Halting system...\n");

				Shutdown(0);
			}

			ActivateDialog(dialogPtr);
		
			HandleLoginDialog(dialogPtr, menuPtr);
			break;
	}
}

void BuildWelcomeString(char *string)
{	
	if (!Ssystem(-1, 0L, 0L))
	{
		char version[12];
		long i, info[] = {0, 0};
		
		for (i = 1; i >= 0; i--)
		{
			info[0] = Ssystem( i, 0L, 0L);
			strcat(string, (char *)info);
		}

		info[0] = Ssystem( 2, 0L, 0L);
		sprintf(version, " %d.%d.%d%c", 
		(int)(info[0]>>24)&0xFF,
		(int)(info[0]>>16)&0xFF, 
		(int)(info[0]>>8)&0xFF, 
		(int)info[0]&0xFF); 
		strcat(string, version);
	}
}

static char *CleanLine(char *buffer)
{
	if (buffer != NULL)
	{
		int lineLength;
		int i;
    
		lineLength = strlen(buffer);

		for (i = 0; i < lineLength; i++)
		{
			if (buffer[i] != kSpace && buffer[i] != kTab)
				break;
		}

		if (i > 0 && i < lineLength)
		{
			lineLength -= i;
			memmove(buffer, buffer + i, lineLength);
		}

		for (i = lineLength; i > 0; i--)
		{
			int j = i - 1;

			if (buffer[j] != kSpace && buffer[j] != kTab)
				break;
		}

		buffer[i] = '\0';
	}

	return buffer;
}

sList *ReadConfig()
{
	FILE *file;

	char filename[] = "//etc//vlogin.conf";
	char *token;
	char *cleanLine;
	char menuItem[32];
	char command[256];
	char line[1024];
	
	int lineCount = 0;

	sCommands *commands;
	sList *menuList = CreateList();
	sList *retVal = NULL;
	
	file = fopen(filename,"r");

	if (!feof(file))
	{
		do
		{
			// get input line    
			cleanLine = CleanLine(fgets(line, sizeof(line), file));
			
			if (cleanLine == NULL) continue;

			lineCount++;

			// skip comments
			if (cleanLine[0] == '#')  continue;

			// get first token
		      token = CleanLine(strtok(line, "=\n\r"));

			if (token != NULL)
			{
				int inside = 0;
				int com = 0;
				int j = 0;
				int i;
				
				menuItem[0] = 0;
				command[0] = 0;

				for (i = 0; i < strlen(token); i++)
				{
					if (token[i] == kCommas)
					{
						i++;
						
						inside ? inside-- : inside++;
					}

					if (!inside && (token[i] == kSpace || token[i] == kTab))
					{
						com = 1;
						j = 0;
					}
					else if (com != 1)
					{
						menuItem[j++] = token[i];
						menuItem[j] = 0;
					}
					else
					{
						command[j++] = token[i];
						command[j] = 0;
					}
				}
				
				commands = malloc(sizeof(sCommands));
				
				commands->menuItem = malloc(sizeof(menuItem));
				strcpy(commands->menuItem, menuItem);

				commands->command = malloc(sizeof(command));
				strcpy(commands->command, command);

				PushBack(menuList, commands);

//				printf("item: %s\ncommand: %s\n", commands->menuItem, commands->command);
				retVal = menuList;
			}

		} while (cleanLine != NULL);
	}

	fclose(file);
	
	return retVal;
}

void SystemReset()
{
	struct utmp *wtmpentry;

	wtmpentry = (struct utmp *)malloc(sizeof(struct utmp));

	if (wtmpentry)
	{
		wtmpentry->ut_type = USER_PROCESS;
		wtmpentry->ut_user[0] = '\0';

	      time(&(wtmpentry->ut_time));

		sscanf(ttyname(0), "/dev/%s", wtmpentry->ut_line);
	#if HAVE_UPDWTMP
		updwtmp("/var/log/wtmp", wtmpentry);
	#endif
      	free(wtmpentry);
	}

	chown(ttyname(0), 0, 0);
	chmod(ttyname(0), 0600);
}
