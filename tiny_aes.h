#ifndef __TINY_AES_H__
#define __TINY_AES_H__


#include "types.h"
#include "list.h"


#define TA_DIALOG			100
#define TA_MOVER			200
#define TA_BOX				300
#define TA_BUTTON			400
#define TA_COMBO_BOX		401	
#define TA_STRING			500
#define TA_EDIT_FIELD		600

#define TA_MENU			1000

#define BUTTON_LEFT			0
#define BUTTON_CENTER		1
#define BUTTON_DEFAULT		2
#define BUTTON_SELECTED		4
#define BUTTON_DISABLED		8

#define FIELD_NORMAL		0
#define FIELD_MASKED		1

#define COMBO_BOX_NORMAL		0
#define COMBO_BOX_SELECTED	1
#define COMBO_BOX_ACTIVATED	2
#define COMBO_BOX_DISABLED	4


typedef struct
{	
	short type;
	void *info;
	sRect coordinates;
	char *string;
} sGraficObject;

typedef struct
{
	sList *dialog;
	sGraficObject *selectedEditField;
	sGraficObject *selectedObject;
	sGraficObject *activatedObject;
} sDialogObject;


// graf handle fo current vdi vorkstation
short handle;

// x, y dimensions of screen, current bpp
short vdiInfo[3];


// entry, exit functins of TAES
void InitTinyAES();
void ExitTinyAES();

short InfoDialog(char *infoStringOne, char *infoStringTwo, char *buttonString);
short AlertDialog(char *alertStringOne, char *alertStringTwo, char *buttonOneString, char *buttonTwoString);

void *CreateDialog(sRect dial_xy, char *label);
void DrawDialog(void *dialog);
void RedrawDialog(void *dialog);
void DisposeDialog(void *dialog);

void RedrawElement(void *dialog, short id);

void ActivateDialog(void *dialog);
void ActivateEditField(void *dialogPtr, void *editField);

void AttachButton(void *dialogPtr, sRect button_xy, int flag, char *label);
void AttachBox(void *dialogPtr, sRect string_xy, char *label);
void AttachString(void *dialogPtr, sRect string_xy, char *label);
void AttachEditField(void *dialogPtr, sRect box_xy, int flag, int size, char *label);
void AttachComboBox(void *dialogPtr, sRect box_xy, int flag, void *menu);

void *CreateMenu();
void DisposeMenu(void *menu);

void SetMenuSelect(void *menu, short item);
short GetMenuSelect(void *menu);

void SetMenuFlag(void *menu, int flag);
short GetMenuFlag(void *menu);

void AttachMenuItem(void *menu, void *item);

#define ever (;;)

#endif
