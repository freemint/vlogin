#include <stdio.h>
#include <fcntl.h>
#include <memory.h>
#include <gem.h>
#include <osbind.h>
#include <mintbind.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/select.h>

#include "tiny_aes.h"
#include "list.h"
#include "vdi_it.h"
#include "debug.h"


// #define MOUSE_VECTOR
#define MOUSE_USE_MOOSE
#define MOUSE_DEVICE_NAME "/dev/mouse"
int mouseHandle = 0;

#if defined(MOUSE_USE_MOOSE)
# include "moose.h"
#endif


// some initialization
void  InitColors();


void  InitMouseVector();
void  RestoreMouseVector();


// prototypes of elements drawing functions 
void  DrawDesktop();
void  DrawButton(void *dialogPtr, sRect button_xy, int flag, char *label, int type);
void  DrawString(void *dialogPtr, sRect string_xy, char *label);
void  DrawEditField(void *dialog, sRect ef_xy, int flag, char *label);
void  DrawComboBox(void *dialog, void *comboBox);
void  DrawMenu(void *menu, sRect menuRect, short item);
void  DrawShades(sRect *rect, short width, short bool);

void  PaintGradient(short handle, sRect xy, short *rgbB, short *rgbE);

void  DrawCursor(short x, short y, short color);
char  PtInRect(short x, short y, sRect rect);

// saving/restoring/moving rects on screen/memmory
void  *FormSave(sRect *xy);
void  FormRestore(sRect *xy, void *area);
void  FormCopy(sRect *from, sRect *to);

// event handling functions
int   EventLoop();

short HandleMouse(short mouseX, short mouseY, short mouseButton);
void  HandleMouseDown(short mouseX, short mouseY);
short HandleMouseUp(short mouseX, short mouseY);
void  HandleMouseMotion(short motionX, short motionY);

short HandleKeyboard(unsigned long keyPressed);
void  HandleActivObject(unsigned char asciiCode, unsigned char scanCode);
void  HandleMenu(unsigned char asciiCode, unsigned char scanCode);

void  SetActivNext(short isTab);
void  SetActivPrev();


// ----------------------------------------------------------------------
// global variables
// ----------------------------------------------------------------------

sList *selectedWindow			= NULL;
sGraficObject *selectedEditField	= NULL;
sGraficObject *selectedObject 	= NULL;
sGraficObject *activatedObject 	= NULL;

short *colors;

short monoColors[6] 	= {1, 0, 1, 1, 0, 1};
short fourColors[6] 	= {3, 0, 1, 1, 0, 1};
short sixteenColors[6] 	= {12, 8, 0, 9, 0, 1};

short applId;
short currentComboBoxValue = 0;

// AES physical wk handle
static short phandle = 0;


void InitColors()
{
	short rgbB[3] = {600, 600, 800};
	short rgbE[3] = {800, 900, 1000};

	if (vdi_inf[2] == 1)
		colors = &monoColors[0];

	if (vdi_inf[2] == 2)
		colors = &fourColors[0];

	if (vdi_inf[2] > 2)
		colors = &sixteenColors[0];

	if(vdi_inf[2] >= 16)
	{
		colors[1] = 221;
		colors[2] = 221;
		colors[3] = 222;
		vs_color(handle, 221, rgbE);
		vs_color(handle, 222, rgbB);
	}
}

void InitMouseVector()
{
#if defined(MOUSE_VECTOR)
	// install our mouse vectors
	vex_motv(handle, VdiMotionVector, &Vdi_oldmotionvector);
	vex_butv(handle, VdiButtonVector, &Vdi_oldbuttonvector);
#elif defined(MOUSE_USE_MOOSE)
	moose_init(phandle);
#else
	// Open device 
	mouseHandle = open(MOUSE_DEVICE_NAME, 0);
	if (mouseHandle<0) {
		printf("Can not open " MOUSE_DEVICE_NAME "\n");
	}
#endif
}

void RestoreMouseVector()
{
#if defined(MOUSE_VECTOR)
	void *dummyPointer;	

	// restore old mouse vectors
	vex_motv(handle, Vdi_oldmotionvector, &dummyPointer);
	vex_butv(handle, Vdi_oldbuttonvector, &dummyPointer);
#elif defined(MOUSE_USE_MOOSE)
	moose_exit(phandle);
#else
	// Open device 
	close(mouseHandle);
#endif
}

/*
 * standard GEM arrow
 */
static MFORM M_ARROW_MOUSE =
{
	0x0000, 0x0000, 0x0001, 0x0000, 0x0001,
	{ /* Mask data */
	0xC000, 0xE000, 0xF000, 0xF800,
	0xFC00, 0xFE00, 0xFF00, 0xFF80,
	0xFFC0, 0xFFE0, 0xFE00, 0xEF00,
	0xCF00, 0x8780, 0x0780, 0x0380 },
	{ /* Cursor data */
	0x0000, 0x4000, 0x6000, 0x7000,
	0x7800, 0x7C00, 0x7E00, 0x7F00,
	0x7F80, 0x7C00, 0x6C00, 0x4600,
	0x0600, 0x0300, 0x0300, 0x0000 }
};

void InitTinyAES()
{
	short vdi_init[11];
	short garbage[57];
	short i;
				
	applId = appl_init();

	for (i = 0; i<10; i++)
		vdi_init[i] = 1;

	vdi_init[10] = 2;

	if (applId >= 0)
	{
#ifdef AES_DEBUG
		// The GEM AES has already been started,
		// so get the physical workstation handle from it
		short junk;
		phandle = graf_handle(&junk, &junk, &junk, &junk);
#else

		fprintf(stderr,"Do not start VDI login from AES!\n");
		ExitTinyAES();
		exit(-1);

#endif
	}
	else
	{
		v_opnwk(vdi_init, &phandle, garbage);
		v_exit_cur(phandle); /* XaAES: Ozk: We need to get rid of the cursor */
	}
	
	if (phandle == 0)
	{
		fprintf(stderr,"Can not open VDI workstation!\n");
		ExitTinyAES();
		exit(-1);
	}

	vdi_inf[0] = garbage[0];
	vdi_inf[1] = garbage[1];
	
	handle = phandle;
	v_opnvwk(vdi_init, &handle, garbage);

	v_hide_c(handle);
	vsc_form(handle, (short *)&M_ARROW_MOUSE); // fVDI!! to show mouse

#ifdef AES_DEBUG
	if (0)
	{
		short pxy[] = {40,50,700,730};
		vs_clip (handle, 1, pxy);
	}
#endif

	vq_extnd(handle, 1, garbage);

	vdi_inf[2] = garbage[4];

	InitColors();
	
	InitMouseVector();

	DrawDesktop();

	v_show_c(handle, 0);
}

void ExitTinyAES()
{
	RestoreMouseVector();

	v_clsvwk(handle);

	if (applId >= 0)
	{
		appl_exit();
	}
	else
	{
		v_enter_cur(phandle);	/* Ozk: Lets enter cursor mode */
		v_clswk(phandle);
	}
}

// ---------------------------------------------------------------------------

short AlertDialog(char *stringOne, char *stringTwo, char *buttonOneString, char *buttonTwoString)
{
	sList *dialogPtr;
	sElement *element;
	sGraficObject *object;

	void *activatedObjectBackup = activatedObject;
	
	short returnValue = -1;

	sRect windowPositon 	= {vdi_inf[0] / 2 - 130, vdi_inf[1] / 2 - 65, vdi_inf[0] / 2 + 130, vdi_inf[1] / 2 + 65};
	sRect buttonOnePosition 	= {62, 98, 122, 118};
	sRect buttonTwoPosition 	= {138, 98, 198, 118};
	sRect boxPosition 		= {16, 35, 244, 86};
	sRect stringOnePositon 	= {24, 56, 0, 0};
	sRect stringTwoPositon 	= {24, 76, 0, 0};

	activatedObject = NULL;

	dialogPtr = (sList *)CreateDialog(windowPositon, "Alert");
	element = (sElement *)dialogPtr->first;
	object = (sGraficObject *)element->data;

	AttachBox(dialogPtr, boxPosition, NULL);
	AttachString(dialogPtr, stringOnePositon, stringOne);
	AttachString(dialogPtr, stringTwoPositon, stringTwo);
	AttachButton(dialogPtr, buttonOnePosition, BUTTON_CENTER + BUTTON_DEFAULT, buttonOneString);
	AttachButton(dialogPtr, buttonTwoPosition, BUTTON_CENTER, buttonTwoString);

	DrawDialog(dialogPtr);

	returnValue = EventLoop();
	DisposeDialog(dialogPtr);

	activatedObject = activatedObjectBackup;

	return returnValue == 5 ? 1: 0;
}

short InfoDialog(char *stringOne, char *stringTwo, char *buttonString)
{
	sList *dialogPtr;
	sElement *element;
	sGraficObject *object;

	void *activatedObjectBackup = activatedObject;
	
	sRect windowPositon = {vdi_inf[0] / 2 - 130, vdi_inf[1] / 2 - 65, vdi_inf[0] / 2 + 130, vdi_inf[1] / 2 + 65};
	sRect buttonPosition = {100, 98, 160, 118};
	sRect boxPosition = {16, 35, 244, 86};
	sRect stringOnePositon = {24, 56, 0, 0};
	sRect stringTwoPositon = {24, 76, 0, 0};
	
	  DEBUG("InfoDialog 1\n");
	activatedObject = NULL;

	dialogPtr = (sList *)CreateDialog(windowPositon, "Info");
	element = (sElement *)dialogPtr->first;
	object = (sGraficObject *)element->data;

	  DEBUG("InfoDialog 2\n");
	AttachBox(dialogPtr, boxPosition, NULL);
	AttachString(dialogPtr, stringOnePositon, stringOne);
	AttachString(dialogPtr, stringTwoPositon, stringTwo);
	AttachButton(dialogPtr, buttonPosition, BUTTON_CENTER + BUTTON_DEFAULT, buttonString);

	  DEBUG("InfoDialog 3\n");
	DrawDialog(dialogPtr);
	  DEBUG("InfoDialog 4\n");

	EventLoop();
	  DEBUG("InfoDialog 5\n");
	DisposeDialog(dialogPtr);
	
	activatedObject = activatedObjectBackup;

	return 1;
}

// ---------------------------------------------------------------------------

void ActivateDialog(void *dialog)
{
	sGraficObject *dialBody = (sGraficObject *)((sElement *)((sList *)dialog)->first)->data;
	void *deselectedWindow = selectedWindow;

	  DEBUG("ActivateDialog 1\n");
	selectedWindow = dialog;

	  DEBUG("ActivateDialog 2\n");
	if(deselectedWindow)
		RedrawDialog(deselectedWindow);

	  DEBUG("ActivateDialog 3\n");
	if (dialBody && !dialBody->info)
		dialBody->info = FormSave(&dialBody->coordinates);

	  DEBUG("ActivateDialog 4\n");
	RedrawDialog(selectedWindow);
	  DEBUG("ActivateDialog/\n");
}

void ActivateEditField(void *dialog, void *currentObject)
{
	sElement		*element = (sElement *)((sList *)dialog)->first;
	sGraficObject	*dialogObject = (sGraficObject *)element->data;
	sGraficObject	*previousObject = (sGraficObject *)activatedObject;
	
	if (activatedObject)
	{
		if (previousObject->type == TA_COMBO_BOX)
		{
			sList	*menuList = (sList *)((sGraficObject *)previousObject->string)->string;
			sRect 	rect = previousObject->coordinates;

			rect.x+= dialogObject->coordinates.x;
			rect.y+= dialogObject->coordinates.y;
			rect.w+= dialogObject->coordinates.x;
			rect.h+= dialogObject->coordinates.y;
		
			rect.y = rect.h + 1; rect.h+= 1 + 17 * (menuList->itemCount - 1);

			SetMenuFlag(previousObject->string, COMBO_BOX_NORMAL);			
			FormRestore(&rect, ((sGraficObject *)previousObject->string)->info);

			DrawComboBox(dialog, previousObject);
		}
		else
		{
			DrawCursor(previousObject->coordinates.x + dialogObject->coordinates.x + (strlen(previousObject->string) * 8) + 2, previousObject->coordinates.y + dialogObject->coordinates.y + 2, 0);
		}
	}
	
	if (currentObject)
	{
		sGraficObject *activObject = ((sGraficObject *)currentObject);

		activatedObject = currentObject;

		if (activObject->type == TA_COMBO_BOX)
		{
			SetMenuFlag(((sGraficObject *)activObject->string), COMBO_BOX_ACTIVATED);			
			DrawComboBox(dialog, activObject);
		}
		else
		{
			DrawCursor(activObject->coordinates.x + dialogObject->coordinates.x + (strlen(activObject->string) * 8) + 2, activObject->coordinates.y + dialogObject->coordinates.y + 2, 1);
		}
	}
}

// ---------------------------------------------------------------------------

void *CreateMenu()
{
	sGraficObject *menu = malloc(sizeof(sGraficObject));
	sList *list = CreateList();
	
	menu->type = TA_MENU;
	menu->string = (char *)list;
//	menu->coordinates = rect;
	menu->info = (void *)0;
	
	PushBack(list, (void *)0);

	return (void *)menu;
}

void DisposeMenu(void *menu)
{
	sList *menuPtr = (sList *)((sGraficObject *)menu)->string;
	sElement *element = (sElement *)menuPtr->first;

	int i;

	if (menu == NULL) return;

	for (i = 0; i < menuPtr->itemCount; i++)
	{
		free(element->data);
		EraseFirst(menuPtr);
		element = (void *)element->next;
	}
	
	DestroyList(menuPtr);
	
	free((sGraficObject *)menu);
}

void SetMenuSelect(void *menu, short item)
{
	sElement *element = ((sList *)((sGraficObject *)menu)->string)->first;
	element->data = (void *)(((int)element->data & 0xffff) | (item << 16));
}

short GetMenuSelect(void *menu)
{
	sElement *element = ((sList *)((sGraficObject *)menu)->string)->first;
	return ((int)element->data) >> 16;
}

void SetMenuFlag(void *menu, int flag)
{
	sElement *element = ((sList *)((sGraficObject *)menu)->string)->first;
	element->data = (void *)(flag | ((((int)element->data) >> 16) << 16));
}

short GetMenuFlag(void *menu)
{
	sElement *element = ((sList *)((sGraficObject *)menu)->string)->first;
	return ((int)element->data) & 0xffff;
}

// ---------------------------------------------------------------------------

void AttachMenuItem(void *menu, void *item)
{
	char *itemCopy;

	if (menu == NULL) return;

	itemCopy = malloc(strlen(item) + 1);
	strcpy(itemCopy, item);
		
	PushBack((sList *)((sGraficObject *)menu)->string, itemCopy);
}

// ---------------------------------------------------------------------------

void AttachComboBox(void *dialogPtr, sRect box_xy, int flag, void *menu)
{
	sGraficObject 	*comboBox;
	
	if (dialogPtr == 0) return;
	
	comboBox = malloc(sizeof(sGraficObject));
	comboBox->type = TA_COMBO_BOX;
	comboBox->info = NULL;
	comboBox->string = (char *)menu;
	comboBox->coordinates = box_xy;

	SetMenuFlag(menu, flag);

	PushBack((sList *)dialogPtr, (void *)comboBox);
}

void AttachEditField(void *dialogPtr, sRect box_xy, int flag, int size, char *label)
{
	sGraficObject 	*editField;
	
	if (dialogPtr == 0) return;

	editField = malloc(sizeof(sGraficObject));

	flag = flag|(size<<16);
	
	editField->type = TA_EDIT_FIELD;
	editField->info = (void *)flag;
	editField->string = label;
	editField->coordinates = box_xy;

	PushBack((sList *)dialogPtr, (void *)editField);
}

void AttachBox(void *dialogPtr, sRect box_xy, char *label)
{
	sGraficObject 	*box;
	char 		*labelCopy;
	
	if (dialogPtr == 0) return;

	box   	= malloc(sizeof(sGraficObject));
	labelCopy = malloc(strlen(label) + 1);

	strcpy(labelCopy, label);

	box->type = TA_BOX;
	box->string = labelCopy;
	box->coordinates = box_xy;

	PushBack((sList *)dialogPtr, (void *)box);
}

void AttachString(void *dialogPtr, sRect string_xy, char *label)
{
	sGraficObject 	*string;
	char 		*labelCopy;
	
	if (dialogPtr == 0) return;

	string  = malloc(sizeof(sGraficObject));
	labelCopy  = malloc(strlen(label) + 1);

	strcpy(title, label);

	string->type = TA_STRING;
	string->string = labelCopy;
	string->coordinates = string_xy;

	PushBack((sList *)dialogPtr, (void *)string);
}

void AttachButton(void *dialogPtr, sRect button_xy, int flag, char *label)
{
	sGraficObject 	*button;
	char 		*labelCopy;
	
	if (dialogPtr == 0) return;
		
	button	= malloc(sizeof(sGraficObject));
	labelCopy = malloc(strlen(label) + 1);

	strcpy(labelCopy, label);

	button->type = TA_BUTTON;
	button->string = labelCopy;
	button->info = (void *)flag;
	button->coordinates = button_xy;
	
	PushBack((sList *)dialogPtr, (void *)button);
}

// ---------------------------------------------------------------------------

void DrawDesktop()
{
	if (vdi_inf[2] == 1)
	{
		short desk[4] = {0, 0, vdi_inf[0], vdi_inf[1]};
	
		vsf_color(handle, colors[0]);
		vsf_style(handle, 4);
		vsf_interior(handle, 2);
		vr_recfl(handle, desk);
		vsf_interior(handle, 1);
	}
	else if ((vdi_inf[2] >= 2) && (vdi_inf[2] <= 8))
	{
		short desk[4] = {0, 0, vdi_inf[0], vdi_inf[1]};
	
		vsf_color(handle, colors[0]);
		vr_recfl(handle, desk);
	}
	else if (vdi_inf[2] >= 15)
	{
		sRect deskRect = {0, 0, vdi_inf[0], vdi_inf[1] + 1};
		short rgbE[3] = {100, 200, 500};
		short rgbB[3] = {0, 0, 200};

		vs_color(handle, 221, rgbE);
		vs_color(handle, 222, rgbB);

		PaintGradient(handle, deskRect, rgbB, rgbE);
	}
}

void DrawButton(void *dialogPtr, sRect button_xy, int flag, char *label, int type)
{
	sList		*list = (sList *)dialogPtr;
	sElement		*element;
	sGraficObject	*object;

	short rgbB[3];
	short rgbE[3];

	short labelLength = strlen(label);

	element = (sElement *)list->first;
	object = (sGraficObject *)element->data;
	
	button_xy.x+= object->coordinates.x;
	button_xy.y+= object->coordinates.y;
	button_xy.w+= object->coordinates.x;
	button_xy.h+= object->coordinates.y;

	v_hide_c(handle);

	vswr_mode(handle, MD_REPLACE);
	
	vq_color(handle, 221, 1, rgbE);
	vq_color(handle, 222, 1, rgbB);

	if (type == TA_DIALOG || type == TA_BOX)
	{
		rgbB[0] = rgbB[1] = rgbB[2] = 800; 
		rgbE[0] = rgbE[1] = rgbE[2] = 950;
	}
	else 
	{
		if (dialogPtr == selectedWindow && ((flag >> 3) & 1) != 1)
		{
			rgbB[0] = 600; rgbB[1] = 600; rgbB[2] = 800;
			rgbE[0] = 800; rgbE[1] = 900; rgbE[2] = 1000;
		}
		else
		{
			rgbB[0] = rgbB[1] = rgbB[2] = 600;
			rgbE[0] = rgbE[1] = rgbE[2] = 800;
		}
	}

	vs_color(handle, 221, rgbE); vs_color(handle, 222, rgbB);
	
	if (((flag >> 2) & 1) != 1 )
		PaintGradient(handle, button_xy, rgbB, rgbE);
	else
		PaintGradient(handle, button_xy, rgbE, rgbB);

	if (((flag >> 1) & 1) == 1 )
	{
		if (((flag >> 2) & 1) != 1 )
			DrawShades(&button_xy, 3, 0);
		else
			DrawShades(&button_xy, 3, 1);
	}
	else
	{
		if (((flag >> 2) & 1) != 1 )
			DrawShades(&button_xy, 2, 0);
		else
			DrawShades(&button_xy, 2, 1);
	}

	vswr_mode(handle, MD_TRANS);

	if (((dialogPtr != selectedWindow) && (type == TA_MOVER)) || (((flag >> 3) & 1) == 1))
		vst_color(handle, colors[3]);

	if (labelLength != 0)
	{
		if ((flag & 1) == 1 )
		{
			if (((flag >> 2) & 1) != 1 )
				v_gtext(handle, button_xy.x + ((button_xy.w - button_xy.x) / 2) - (labelLength * 4), button_xy.y + (((button_xy.h - button_xy.y) / 2) - 8) + 14, label);
			else
				v_gtext(handle, button_xy.x + ((button_xy.w - button_xy.x) / 2) - (labelLength * 4) + 1, button_xy.y + (((button_xy.h - button_xy.y) / 2) - 8) + 15, label);
		}
		else
		{
			if (((flag >> 2) & 1) != 1 )
				v_gtext(handle, button_xy.x + 8, button_xy.y + (((button_xy.h - button_xy.y) / 2) - 8) + 14, label);
			else
				v_gtext(handle, button_xy.x + 9, button_xy.y + (((button_xy.h - button_xy.y) / 2) - 8) + 15, label);
		}
	}

	vswr_mode( handle, MD_REPLACE);
	vst_color(handle, colors[5]);

	v_show_c(handle, 0);

	rgbB[0] = rgbB[1] = rgbB[2] = 800;
	rgbE[0] = rgbE[1] = rgbE[2] = 950;

	vs_color(handle, 221, rgbE); vs_color(handle, 222, rgbB);
}

void DrawString(void *dialog, sRect string_xy, char *label)
{
	sElement		*element = (sElement *)((sList *)dialog)->first;
	sGraficObject	*object = (sGraficObject *)element->data;
	
	string_xy.x+= object->coordinates.x;
	string_xy.y+= object->coordinates.y;

	vswr_mode(handle, MD_TRANS);
	v_hide_c(handle);

	v_gtext(handle, string_xy.x, string_xy.y, label);

	v_show_c(handle, 0);
	vswr_mode(handle, MD_REPLACE);
}

void DrawEditField(void *dialog, sRect ef_xy, int flag, char *label)
{
	sElement		*element = (sElement *)((sList *)dialog)->first;
	sGraficObject	*object = (sGraficObject *)element->data;
	short		xy[4];

	if ((flag & 1) == 1)
	{
		int i;
		int labelLen = strlen(label); 

		label = malloc(labelLen + 1);
		label[labelLen] = 0;

		for (i = 0; i < labelLen; i++)
			label[i] = '*';
	}

	xy[0] = ef_xy.x+= object->coordinates.x;
	xy[1] = ef_xy.y+= object->coordinates.y;
	xy[2] = ef_xy.w+= object->coordinates.x;
	xy[3] = ef_xy.h+= object->coordinates.y;

	v_hide_c(handle);

	vsf_color(handle, colors[4]);
	v_bar(handle, xy);

	DrawShades(&ef_xy, 1, 1);

	v_gtext(handle, ef_xy.x + 2, ef_xy.y + (((ef_xy.h - ef_xy.y) / 2) - 8) + 14, label);

	v_show_c(handle, 0);

	if ((flag & 1) == 1)
		free(label);
}

void DrawComboBox(void *dialog, void *comboBox)
{
	sGraficObject	*menuObject = (sGraficObject *)((sGraficObject *)comboBox)->string;
	sList		*menuPtr = (sList *)menuObject->string;
	sElement		*element = (sElement *)((sList *)dialog)->first;
	sGraficObject	*dialogObject = (sGraficObject *)element->data;
	sRect		boxCoordinates = {((sGraficObject *)comboBox)->coordinates.x, ((sGraficObject *)comboBox)->coordinates.y, ((sGraficObject *)comboBox)->coordinates.w, ((sGraficObject *)comboBox)->coordinates.h};
	sRect 		buttonRect = {((sGraficObject *)comboBox)->coordinates.w - 19, ((sGraficObject *)comboBox)->coordinates.y + 1, ((sGraficObject *)comboBox)->coordinates.w - 1, ((sGraficObject *)comboBox)->coordinates.h - 1};
	char			*string;
	short 		xy[4];
	int			i;

	xy[0] = boxCoordinates.x += dialogObject->coordinates.x;
	xy[1] = boxCoordinates.y += dialogObject->coordinates.y;
	xy[2] = boxCoordinates.w += dialogObject->coordinates.x;
	xy[3] = boxCoordinates.h += dialogObject->coordinates.y;

	v_hide_c(handle);

	vsf_color(handle, colors[4]);
	v_bar(handle, xy);

	DrawShades(&boxCoordinates, 1, 1);

	if (menuPtr)
	{
		element = (sElement *)menuPtr->first;

		for (i = 0; i <= GetMenuSelect(((sGraficObject *)comboBox)->string); i++)
			element = (void *)element->next;

		string = element != 0 ? (char *)element->data : 0;

		v_gtext(handle, xy[0] + 2, xy[1] + (((xy[3] - xy[1]) / 2) - 8) + 14, string);
	}

	switch (GetMenuFlag(((sGraficObject *)comboBox)->string))
	{
		case COMBO_BOX_NORMAL:
			// printf("Normal\n");
			DrawButton(dialog, buttonRect, BUTTON_CENTER, "", TA_BUTTON);
			menuObject->info = NULL;
			break;

		case COMBO_BOX_SELECTED:
			// printf("Selected\n");
			DrawButton(dialog, buttonRect, BUTTON_CENTER + BUTTON_SELECTED, "", TA_BUTTON);
			menuObject->info = NULL;
			break;

		case COMBO_BOX_DISABLED:
			// printf("Disabled\n");
			DrawButton(dialog, buttonRect, BUTTON_CENTER + BUTTON_DISABLED, "", TA_BUTTON);
			menuObject->info = NULL;
			break;

		case COMBO_BOX_ACTIVATED:
			{
			sRect menuRect = {xy[0], xy[3] + 1, xy[2], xy[3] + 1 + 17 * (menuPtr->itemCount - 1)};

			// printf("Activated\n");
			DrawButton(dialog, buttonRect, BUTTON_CENTER + BUTTON_SELECTED, "", TA_BUTTON);

			if (menuPtr)
			{
				if (menuObject->info == NULL)
					menuObject->info = FormSave(&menuRect);

				DrawMenu(menuPtr, menuRect, GetMenuSelect(((sGraficObject *)comboBox)->string));
			}
			break;
			}
	}

	v_show_c(handle, 0);
}

void DrawMenu(void *menu, sRect menuRect, short item)
{
	sList *menuPtr = (sList *)menu;
	sElement *menuElement = (sElement *)menuPtr->first;
	
	char *string;
	
	int i;

	short rgbB[3];
	short rgbE[3];

	v_hide_c(handle);

	vq_color(handle, 221, 1, rgbE);
	vq_color(handle, 222, 1, rgbB);

	PaintGradient(handle, menuRect, rgbB, rgbE);

	DrawShades(&menuRect, 1, 0);

	menuElement = (void *)menuElement->next;
	string = (char *)menuElement->data;

	for (i = 0; i < menuPtr->itemCount - 1; i++)
	{
		vswr_mode(handle, MD_TRANS);
		if (string && string[0] == '-')
		{
			short line[4] = {menuRect.x + 4, (menuRect.y + 8) + (i * 17), menuRect.w - 4, (menuRect.y + 8) + (i * 17)};

			vsl_color(handle, colors[3]);
			v_pline(handle, 2, line);
			line[1]++; line[3]++;
			vsl_color(handle, colors[2]);
			v_pline(handle, 2, line);
		}
		else
		{
			v_gtext(handle, menuRect.x + 5, (menuRect.y + 14) + (i * 17), string);
		}

		if (item == i)
		{
			short cursor[4] = {menuRect.x + 2, menuRect.y + (i * 17), menuRect.w -2, (menuRect.y + 17) + (i * 17)};

			vswr_mode(handle, MD_XOR);

			vsf_color(handle, colors[5]);
			v_bar(handle, cursor);
		}			

		vswr_mode(handle, MD_REPLACE);

		if (menuElement->next)
		{
			menuElement = (void *)menuElement->next;
			string = menuElement != 0 ? (char *)menuElement->data : 0;
		}
	}

	v_show_c(handle, 0);
}

void DrawCursor(short x, short y, short color)
{
	short cursor_xy[4] = {x, y, x + 8, y + 16};
	
	v_hide_c(handle);
	vsf_color(handle, color);
	v_bar(handle, cursor_xy);
	v_show_c(handle, 0);
}

void DrawShades(sRect *rect, short width, short bool)
{
	short shadeTop[6] = {rect->x, rect->h, rect->w, rect->h, rect->w, rect->y};
	short shadeBtm[6] = {rect->x, rect->h, rect->x, rect->y, rect->w, rect->y};

	int i;
	
	for (i = 0; i < width; i++)
	{
		vsl_color(handle, colors[3 - bool]);
		v_pline(handle, 3, shadeTop);
		vsl_color(handle, colors[2 + bool]);
		v_pline(handle, 3, shadeBtm);

		shadeTop[0]++; shadeTop[1]--; shadeTop[2]--;
		shadeTop[3]--; shadeTop[4]--; shadeTop[5]++;

		shadeBtm[0]++; shadeBtm[1]--; shadeBtm[2]++;
		shadeBtm[3]++; shadeBtm[4]--; shadeBtm[5]++;
	}
}

// ---------------------------------------------------------------------------

void *CreateDialog(sRect dial_xy, char *label)
{
	sList *list;
	char  *labelCopy;

	sGraficObject *dialog = malloc(sizeof(sGraficObject));
	sGraficObject *mover = malloc(sizeof(sGraficObject));
	
	list = CreateList();
	if (list == 0) return NULL;
	
	labelCopy = malloc(strlen(label) + 1);
	strcpy(labelCopy, label);

	v_hide_c(handle);

	dialog->type = TA_DIALOG;
	dialog->string = NULL;
	dialog->coordinates = dial_xy;
	dialog->info = NULL;
//	dialog->info = FormSave(&dialog->coordinates);

	v_show_c(handle, 0);

	dial_xy.x = 2;
	dial_xy.y = 2;
	dial_xy.w-= dialog->coordinates.x + 2;
	dial_xy.h = dial_xy.y + 21;

	mover->type = TA_MOVER;
	mover->info = (void *)BUTTON_LEFT;
	mover->string = label;
	mover->coordinates = dial_xy;

	PushBack(list, (void *)dialog);
	PushBack(list, (void *)mover);

	return (void *)list;
}

void DrawDialog(void *dialog)
{
	ActivateDialog(dialog);
}

void RedrawDialog(void *dialog)
{
	sElement 		*element = (sElement *)((sList *)dialog)->first;
	sGraficObject 	*object = (sGraficObject *)element->data;
	sRect 		dialRect;

	int 			i;

	for (i = 0; i < ((sList *)dialog)->itemCount; i++)
	{
		switch (object->type)
		{
			case TA_DIALOG: // dialog
				dialRect.x = 0;
				dialRect.y = 0;
				dialRect.w = object->coordinates.w - object->coordinates.x;
				dialRect.h = object->coordinates.h - object->coordinates.y;

				DrawButton(dialog, dialRect, 0, object->string, object->type);
			
				break;
			case TA_MOVER: // mover
				DrawButton(dialog, object->coordinates, (int)object->info, object->string, object->type);
			
				break;
			case TA_BOX: // box
				DrawButton(dialog, object->coordinates, BUTTON_SELECTED, object->string, object->type);
			
				break;
			case TA_BUTTON: // button
				DrawButton(dialog, object->coordinates, (int)object->info, object->string, object->type);
			
				break;
			case TA_STRING: // string
				DrawString(dialog, object->coordinates, object->string);
			
				break;
			case TA_EDIT_FIELD: // edit field
				DrawEditField(dialog, object->coordinates, (int)object->info, object->string);

				if (!activatedObject)
					ActivateEditField(dialog, object);
			
				break;
			case TA_COMBO_BOX:
				DrawComboBox(dialog, object);
				break;
		}	

		element = (void *)element->next;
		object = element != 0 ? (sGraficObject *)element->data : 0;
	}
}

void DisposeDialog(void *dialog)
{
	sElement *element = (sElement *)((sList *)dialog)->first;
	sGraficObject *object = (sGraficObject *)element->data;

	int i;

	// restore dialog background
	v_hide_c(handle);
	FormRestore(&object->coordinates, object->info);
	v_show_c(handle, 0);
	
	// travers dialog items list and free its memory
	for (i = 0; i < ((sList *)dialog)->itemCount; i++)
	{
		if (object->type == TA_MOVER || object->type == TA_BOX ||
			object->type == TA_BUTTON || object->type == TA_STRING)
			free(object->string);

		free(object);

		EraseFirst(dialog);

		element = (sElement *)((sList *)dialog)->first;
		object = element ? (sGraficObject *)element->data : 0;
	}
	
	if (dialog == selectedWindow)
		selectedWindow = 0;

	DestroyList(dialog);
}

// --------------------------------------------------------------------------

void RedrawElement(void *dialog, short id)
{
	sList *dialogPtr = (sList *)dialog;
	sElement *element;
	sGraficObject *object;
	
	int i;

	if (dialogPtr == 0)
		return;

	if (dialogPtr->itemCount < id + 2)
		return;

	element = (sElement *)dialogPtr->first;
	object = (sGraficObject *)element->data;

	for (i = 0; i < id + 2; i++)
	{
		element = (void *)element->next;
		object = element != 0 ? (sGraficObject *)element->data : 0;
	}

	switch (object->type)
	{
		case TA_BOX: // box
			DrawButton(dialog, object->coordinates, BUTTON_SELECTED, object->string, object->type);
			
			break;
		case TA_BUTTON: // button
			DrawButton(dialog, object->coordinates, (int)object->info, object->string, object->type);
			
			break;
		case TA_STRING: // string
			DrawString(dialog, object->coordinates, object->string);
			
			break;
		case TA_EDIT_FIELD: // edit field
			DrawEditField(dialog, object->coordinates, (int)object->info, object->string);

			if (object == activatedObject)
				ActivateEditField(dialogPtr, object);
			
			break;
		case TA_COMBO_BOX:
			DrawComboBox(dialog, object);
			
			break;
	}	
}

// ---------------------------------------------------------------------------

void PaintGradient(short handle, sRect xy, short *rgbB, short *rgbE)
{
	short 	xywh[4];
	short	currentColour[3];
	float	firstColour[3] = {rgbB[0], rgbB[1], rgbB[2]};
	float	lastColour[3]  = {rgbE[0], rgbE[1], rgbE[2]};

	if(vdi_inf[2] >= 16)
	{
		int i;

		for (i = 0; i < xy.h - xy.y; i++)
		{
			currentColour[0] = firstColour[0] > lastColour[0] ? 
					firstColour[0] - ((firstColour[0] - lastColour[0]) / (xy.h - xy.y)) * i :
					(firstColour[0] < lastColour[0] ? 
					firstColour[0] + ((lastColour[0] - firstColour[0]) / (xy.h - xy.y)) * i : firstColour[0]);
			currentColour[1] = firstColour[1] > lastColour[1] ? 
					firstColour[1] - ((firstColour[1] - lastColour[1]) / (xy.h - xy.y)) * i :
					(firstColour[1] < lastColour[1] ?
					firstColour[1] + ((lastColour[1] - firstColour[1]) / (xy.h - xy.y)) * i : firstColour[1]);
			currentColour[2] = firstColour[2] > lastColour[2] ?
					firstColour[2] - ((firstColour[2] - lastColour[2]) / (xy.h - xy.y)) * i :
					(firstColour[2] < lastColour[2] ?
					firstColour[2] + ((lastColour[2] - firstColour[2]) / (xy.h - xy.y)) * i : firstColour[2]);
		
			vs_color(handle, 220, currentColour);
			vsl_color(handle,220);

			xywh[0] = xy.x;
			xywh[2] = xy.w;
			xywh[1] = xy.y + i;
			xywh[3] = xy.y + i;

			v_pline(handle, 2, xywh);
		}
	}
	else
	{
		xywh[0] = xy.x;
		xywh[1] = xy.y;
		xywh[2] = xy.w;
		xywh[3] = xy.h;

		vsf_color(handle, colors[1]);
		v_bar(handle, xywh);	
	}	
}

char PtInRect(short x, short y, sRect rect) 
{
	if ((x > rect.x) && (x < rect.w) && (y > rect.y) && (y < rect.h))
		return 1;

	else
		return 0;
}

// ---------------------------------------------------------------------------

void *FormSave(sRect *xy)
{
	MFDB Mpreserve;

	DEBUG("FormSave %d,%d,%d,%d\n", xy->x, xy->y, xy->w, xy->h);

	Mpreserve.fd_w = xy->w - xy->x + 1;
	Mpreserve.fd_h = xy->h - xy->y + 1;
	Mpreserve.fd_wdwidth = (Mpreserve.fd_w + 15) / 16;
	Mpreserve.fd_stand = 0;
	Mpreserve.fd_nplanes = vdi_inf[2];
	Mpreserve.fd_addr = malloc((long)Mpreserve.fd_wdwidth * Mpreserve.fd_h * Mpreserve.fd_nplanes);

	if (Mpreserve.fd_addr != NULL)
	{
		MFDB Mscreen = {0};
		short pnt[8] = {xy->x, xy->y, xy->w, xy->h,
				0, 0, Mpreserve.fd_w - 1, Mpreserve.fd_h - 1};

		v_hide_c(handle);

		DEBUG("FormSave vro_cpyfm\n");
		vro_cpyfm(handle, S_ONLY, pnt, &Mscreen, &Mpreserve);
		DEBUG("FormSave vro_cpyfm/\n");

		v_show_c(handle, 0);
	}

	return Mpreserve.fd_addr;
}

void FormRestore(sRect *xy, void *area)
{
	MFDB Mpreserve;

	Mpreserve.fd_addr = area;
	Mpreserve.fd_w = xy->w - xy->x + 1;
	Mpreserve.fd_h = xy->h - xy->y + 1;
	Mpreserve.fd_wdwidth = (Mpreserve.fd_w + 15) / 16;
	Mpreserve.fd_nplanes = vdi_inf[2];
	Mpreserve.fd_stand = 0;

	if (Mpreserve.fd_addr != 0) {
		MFDB Mscreen = {0};
		short pnt[8] = {0, 0, Mpreserve.fd_w - 1, Mpreserve.fd_h - 1,
				xy->x, xy->y, xy->w, xy->h};

		v_hide_c(handle);
		vro_cpyfm(handle, S_ONLY, pnt, &Mpreserve, &Mscreen);
		v_show_c(handle, 0);

		free(Mpreserve.fd_addr);
	}
}

void FormCopy(sRect *from, sRect *to)
{
	short pnt[8] = {from->x, from->y, from->w, from->h, to->x, to->y, to->w, to->h};
	MFDB Mscreen={0};
	
	v_hide_c(handle);
	vro_cpyfm(handle, S_ONLY, pnt, &Mscreen, &Mscreen);
	v_show_c(handle, 0);
}


// -----------------------------------------------------------------------
// Event handling functions
// -----------------------------------------------------------------------


int EventLoop()
{
	unsigned short previousMouseX      = 0;
	unsigned short previousMouseY      = 0;
	unsigned short previousMouseButton = 0;

	short returnValue = 0;
	long kbdDev;
	
	if (selectedWindow == 0)
		return -1;

	kbdDev = open("u:\\dev\\console", O_DENYRW|O_RDONLY);
	if (kbdDev < 0) return -1;

	Vdi_mousex = Vdi_mousey = Vdi_mouseb = 0;

	for(;;)
	{
#if !defined(MOUSE_USE_MOOSE)
		// read and handle key event, when needed key press occures
		if (Cconis())
			returnValue = HandleKeyboard(Cnecin());
#endif

#if defined(MOUSE_VECTOR)
			// everything is done in the VDI hook
#elif defined(MOUSE_USE_MOOSE)
		struct moose_data mdata;

		{
			fd_set rdfds;
			struct timeval tv;

			FD_ZERO (&rdfds);
			FD_SET (mouseDevHandle,&rdfds);
			FD_SET (kbdDev,&rdfds);

			tv.tv_sec=0;
			tv.tv_usec=500;

#define max(a,b) ((a)>(b)?(a):(b))
			// check whether there are some mouse events
                	if (select(max(mouseDevHandle,kbdDev)+1,&rdfds,NULL,NULL,&tv)) {
				long n;

				if (FD_ISSET(kbdDev,&rdfds))
					returnValue = HandleKeyboard(Cnecin());
					
				if (FD_ISSET(mouseDevHandle,&rdfds)) {
					n = read(mouseDevHandle, &mdata, sizeof(mdata));
					if (n == sizeof(mdata)) switch (mdata.ty) {
						case MOOSE_BUTTON_PREFIX:
							Vdi_mouseb = mdata.state;
							/* fallthrough */
						case MOOSE_MOVEMENT_PREFIX:
							Vdi_mousex = mdata.x;
							Vdi_mousey = mdata.y;
					}
				}
			}
		}
#else
		{
			fd_set rdfds;
			struct timeval tv;

			FD_ZERO (&rdfds);
			FD_SET (mouseHandle,&rdfds);

			tv.tv_sec=0;
			tv.tv_usec=0;

			// check whether there are some mouse events
                	if (select(mouseHandle+1,&rdfds,NULL,NULL,&tv)) {
				char buffer[3];
				/* Read new mouse info */
				if (read(mouseHandle, buffer, 3)>0) {
					Vdi_mouseb = buffer[0];
					Vdi_mousex += (short) ((char) buffer[1]);
					Vdi_mousey += (short) ((char) buffer[2]);
				}
			}
		}
#endif // MOUSE_VECTOR

		// mouse motion 
		if ((Vdi_mousex != previousMouseX) || (Vdi_mousey != previousMouseY))
		{
			short motionX = Vdi_mousex - previousMouseX;
			short motionY = Vdi_mousey - previousMouseY;

			DEBUG("moose: Mousemove %d,%d\n", motionX, motionY);

			previousMouseX = Vdi_mousex;
			previousMouseY = Vdi_mousey;

			if (selectedObject)
			{
				if (selectedObject->type == TA_MOVER)
					HandleMouseMotion(motionX, motionY);
			}
		}

		// mouse button event
		while (Vdi_mouseb != previousMouseButton)
		{
			int i;

			for (i = 0; i < 2; i++)
			{
				int currbutton, prevbutton;

				currbutton = Vdi_mouseb & (1 << i);
				prevbutton = previousMouseButton & (1 << i);
			
				DEBUG("moose: Button %d->%d\n", prevbutton, currbutton);

				if (currbutton && !prevbutton)
					returnValue = HandleMouse(Vdi_mousex, Vdi_mousey, 1);

				if (!currbutton && prevbutton)
					returnValue = HandleMouse(Vdi_mousex, Vdi_mousey, 0);

			}

			previousMouseButton = Vdi_mouseb;

#if defined(MOUSE_USE_MOOSE)
			if ( mdata.state && !mdata.cstate ) {
				/* Ozk: If we get a button packet in which the button
				 * state at the time moose sent it is 'released', we
				 * need to fake a 'button-released' event.
				 * If state == cstate != 0, moose will send a
				 * 'button-released' packet.
				 */ 
				Vdi_mouseb = 0;
				continue;
			}
#endif

			break;
		}

		if (returnValue != 0)
			break;
	}
	
	if (kbdDev >= 0)
		close( kbdDev );

	return returnValue;
}

short HandleMouse(short mouseX, short mouseY, short mouseButton)
{
	sElement *element = (sElement *)selectedWindow->first;
	sGraficObject *object = (sGraficObject *)element->data;
	
	if (activatedObject)
	{
		if (activatedObject->type == TA_COMBO_BOX)
		{
			sList *menuPtr = (sList *)((sGraficObject *)activatedObject->string)->string;

			sRect menuRect;

			menuRect.x = activatedObject->coordinates.x + object->coordinates.x;
			menuRect.y = activatedObject->coordinates.h + object->coordinates.y + 1;
			menuRect.w = activatedObject->coordinates.w + object->coordinates.x;
			menuRect.h = activatedObject->coordinates.h + object->coordinates.y + 1 + (17 * (menuPtr->itemCount - 1));

			if (mouseButton)
			{
				if (PtInRect(mouseX, mouseY, menuRect))
				{
					SetMenuSelect(activatedObject->string, ((mouseY - menuRect.y) / 17));
					DrawComboBox(selectedWindow, activatedObject);
					currentComboBoxValue = GetMenuSelect(activatedObject->string);
				}
			}
			else
			{
				if (selectedEditField)
					ActivateEditField(selectedWindow, selectedEditField);

				else
					ActivateEditField(selectedWindow, NULL);
			}

			return 0;
		}
	}

	if (mouseButton)
		HandleMouseDown(mouseX, mouseY);
	else
		return HandleMouseUp(mouseX, mouseY);

	return 0;
}

void HandleMouseDown(short mouseX, short mouseY)
{
	sElement *element = (sElement *)selectedWindow->first;
	sGraficObject *object = (sGraficObject *)element->data;

	short i;

	for (i = 0; i < selectedWindow->itemCount; i++)
	{
		if (object->type == TA_DIALOG)
		{
			mouseX-= object->coordinates.x;
			mouseY-= object->coordinates.y;
		}

		if (object->type == TA_MOVER)
		{
			if (PtInRect(mouseX, mouseY, object->coordinates))
			{
				DrawButton(selectedWindow, object->coordinates, (int)object->info += BUTTON_SELECTED, object->string, object->type);

				break;
			}
		}

		if (object->type == TA_BUTTON)
		{
			if (PtInRect(mouseX, mouseY, object->coordinates) && ((((int)object->info) >> 3) & 1) != 1)
			{
				DrawButton(selectedWindow, object->coordinates, (int)object->info += BUTTON_SELECTED, object->string, object->type);

				break;
			}
		}

		if (object->type == TA_COMBO_BOX)
		{
			if (PtInRect(mouseX, mouseY, object->coordinates) && (GetMenuFlag((void *)object->string) != COMBO_BOX_DISABLED))
			{
				SetMenuFlag(object->string, COMBO_BOX_SELECTED);
				DrawComboBox(selectedWindow, object);

				break;
			}
		}	

		element = (void *)element->next;
		object = element != 0 ? (sGraficObject *)element->data : 0;
	}

	selectedObject = object;
}

short HandleMouseUp(short mouseX, short mouseY)
{
	sElement *element = (sElement *)selectedWindow->first;
	sGraficObject *object = (sGraficObject *)element->data;

	short returnValue = 0;
	short i;

	for (i = 0; i < selectedWindow->itemCount; i++)
	{
		switch (object->type)
		{
			case TA_DIALOG: // dialog
				mouseX-= object->coordinates.x;
				mouseY-= object->coordinates.y;

				break;

			case TA_BUTTON: // button
				if (PtInRect(mouseX, mouseY, object->coordinates) && object == selectedObject && ((((int)object->info) >> 3) & 1) != 1)
				{ 
					returnValue = i;
					i = selectedWindow->itemCount;
				}

				break;

			case TA_EDIT_FIELD: // edit field
				if (PtInRect(mouseX, mouseY, object->coordinates))
					ActivateEditField(selectedWindow, object);

				break;

			case TA_COMBO_BOX:
				if (PtInRect(mouseX, mouseY, object->coordinates) && object == selectedObject && (GetMenuFlag((void *)object->string) != COMBO_BOX_DISABLED))
				{
					if (((sGraficObject *)activatedObject)->type == TA_EDIT_FIELD)
						selectedEditField = activatedObject;
					else
						selectedEditField = NULL;

					ActivateEditField(selectedWindow, object);
				}
				break;
		}	

		element = (void *)element->next;
		object = element != 0 ? (sGraficObject *)element->data : 0;
	}

	if (selectedObject)
	{
		if (selectedObject->type == TA_MOVER || selectedObject->type == TA_BUTTON)
		{
			DrawButton(selectedWindow, selectedObject->coordinates, (int)selectedObject->info -= BUTTON_SELECTED, selectedObject->string, selectedObject->type);
		}
		else if (selectedObject->type == TA_COMBO_BOX && selectedObject != activatedObject)
		{
			SetMenuFlag(selectedObject->string, COMBO_BOX_NORMAL);
			DrawComboBox(selectedWindow, selectedObject);
		}
	}

	selectedObject = NULL;

	return returnValue;
}

void  HandleMouseMotion(short motionX, short motionY)
{
/*	sRect xy;

	xy.x = ((sGraficObject *)((sElement *)selectedWindow->first)->data)->coordinates.x;
	xy.w = ((sGraficObject *)((sElement *)selectedWindow->first)->data)->coordinates.w;
	xy.y = ((sGraficObject *)((sElement *)selectedWindow->first)->data)->coordinates.y;
	xy.h = ((sGraficObject *)((sElement *)selectedWindow->first)->data)->coordinates.h;

	if (xy.y < 0)
	{
		xy.h -= xy.y;
		xy.y  = 0;
	}
*/
	// printf("mouse  x: %d, mouse  y: %d\n", Vdi_mousex, Vdi_mousey); 
//	printf("motion x: %d, motion y: %d\n", Vdi_mousex - previousMouseX, Vdi_mousey - previousMouseY); 
	FormRestore(&((sGraficObject *)((sElement *)selectedWindow->first)->data)->coordinates, ((sGraficObject *)((sElement *)selectedWindow->first)->data)->info);

	if ((((sGraficObject *)((sElement *)selectedWindow->first)->data)->coordinates.x + motionX) < 0)
		motionX += ((((sGraficObject *)((sElement *)selectedWindow->first)->data)->coordinates.x + motionX) * ( - 1));

	if ((((sGraficObject *)((sElement *)selectedWindow->first)->data)->coordinates.y + motionY) < 0)
		motionY += ((((sGraficObject *)((sElement *)selectedWindow->first)->data)->coordinates.y + motionY) * ( -1 ));

	((sGraficObject *)((sElement *)selectedWindow->first)->data)->coordinates.x += motionX;
	((sGraficObject *)((sElement *)selectedWindow->first)->data)->coordinates.y += motionY;
	((sGraficObject *)((sElement *)selectedWindow->first)->data)->coordinates.w += motionX;
	((sGraficObject *)((sElement *)selectedWindow->first)->data)->coordinates.h += motionY;
/*
	xy.x = ((sGraficObject *)((sElement *)selectedWindow->first)->data)->coordinates.x;
	xy.w = ((sGraficObject *)((sElement *)selectedWindow->first)->data)->coordinates.w;
	xy.y = ((sGraficObject *)((sElement *)selectedWindow->first)->data)->coordinates.y;
	xy.h = ((sGraficObject *)((sElement *)selectedWindow->first)->data)->coordinates.h;

	if (xy.y < 0)
	{
		xy.h -= xy.y;
		xy.y  = 0;
	}
*/
	((sGraficObject *)((sElement *)selectedWindow->first)->data)->info = FormSave(&((sGraficObject *)((sElement *)selectedWindow->first)->data)->coordinates);
	DrawDialog(selectedWindow);
}

short HandleKeyboard(unsigned long keyPressed)
{
	sElement *element = (sElement *)selectedWindow->first;
	sGraficObject *object = (sGraficObject *)element->data;
	
	unsigned char asciiCode = keyPressed & 0xff;
	unsigned char scanCode  = (keyPressed >> 16) & 0xff;
	short bool = activatedObject != 0 ? activatedObject->type : 0;

	if (asciiCode == 27)
	{
		ExitTinyAES();
		exit(0);
	}
	else if (asciiCode == 13 && bool != TA_COMBO_BOX)
	{
		short i;
				
		for (i = 0; i < selectedWindow->itemCount; i++)
		{
			if (object->type == TA_BUTTON && (((int)(object->info)>>1)&1) == 1)
			{
				DrawButton(selectedWindow, object->coordinates, (int)object->info + BUTTON_SELECTED, object->string, object->type);
				sleep(1);
				DrawButton(selectedWindow, object->coordinates, (int)object->info, object->string, object->type);

				return i;
			}
			
			element = (void *)element->next;
			object = element != 0 ? (sGraficObject *)element->data : 0;
		}
	}

	if (activatedObject != 0)
	{
		if (activatedObject->type == TA_EDIT_FIELD)
			HandleActivObject(asciiCode, scanCode);

		else if (activatedObject->type == TA_COMBO_BOX)
			HandleMenu(asciiCode, scanCode);

	}

	return 0;
}

void HandleActivObject(unsigned char asciiCode, unsigned char scanCode)
{
	sElement *element = (sElement *)selectedWindow->first;
	sGraficObject *object = (sGraficObject *)element->data;

	if (asciiCode > 32 && asciiCode < 127)
	{
		char character[] = {asciiCode, 0};

		if (strlen(activatedObject->string) < (int)activatedObject->info >> 16)
		{
			DrawCursor(activatedObject->coordinates.x + object->coordinates.x + (strlen(activatedObject->string) * 8) + 2, activatedObject->coordinates.y + object->coordinates.y + 2, 0);

			v_hide_c(handle);

			if (((int)(activatedObject->info) & 1) == 1 )
				v_gtext(handle, activatedObject->coordinates.x + object->coordinates.x + (strlen(activatedObject->string) * 8) + 2, activatedObject->coordinates.y + object->coordinates.y + 14, "*");
			else
				v_gtext(handle, activatedObject->coordinates.x + object->coordinates.x + (strlen(activatedObject->string) * 8) + 2, activatedObject->coordinates.y + object->coordinates.y + 14, &character[0]);
			
			v_show_c(handle, 0);

			sprintf(&activatedObject->string[strlen(activatedObject->string)], "%c", asciiCode);
			DrawCursor(activatedObject->coordinates.x + object->coordinates.x + (strlen(activatedObject->string) * 8) + 2, activatedObject->coordinates.y + object->coordinates.y + 2, 1);
		}
	}
	else if (asciiCode == 8 && activatedObject != 0)
	{
		if (strlen(activatedObject->string) > 0)
		{
			DrawCursor(activatedObject->coordinates.x + object->coordinates.x + (strlen(activatedObject->string) * 8) + 2, activatedObject->coordinates.y + object->coordinates.y + 2, 0);
			activatedObject->string[strlen(activatedObject->string) - 1] = 0;
			DrawCursor(activatedObject->coordinates.x + object->coordinates.x + (strlen(activatedObject->string) * 8) + 2, activatedObject->coordinates.y + object->coordinates.y + 2, 1);
		}
	}
	else if (asciiCode == 9)
	{
		SetActivNext(1);
	}
	else if (scanCode == 80)
	{
		SetActivNext(0);
	}
	else if (scanCode == 72)
	{
		SetActivPrev();
	}
}

void HandleMenu(unsigned char asciiCode, unsigned char scanCode)
{
	if (asciiCode == 9)
	{
		SetMenuSelect(activatedObject->string, currentComboBoxValue);
			
		SetActivNext(1);
	}
	else if (asciiCode == 13)
	{
		currentComboBoxValue = GetMenuSelect(activatedObject->string);

		if (selectedEditField)
			ActivateEditField(selectedWindow, selectedEditField);
		else
			ActivateEditField(selectedWindow, NULL);
	}
	else if (scanCode == 80)
	{
		sList *menu = (sList *)((sGraficObject *)activatedObject->string)->string;

		int item = GetMenuSelect(activatedObject->string);
	
		if (++item >= (menu->itemCount - 1))
			item = 0;

		SetMenuSelect(activatedObject->string, item);
		DrawComboBox(selectedWindow, activatedObject);
	}
	else if (scanCode == 72)
	{
		sList *menu = (sList *)((sGraficObject *)activatedObject->string)->string;

		int item = GetMenuSelect(activatedObject->string);
	
		if (--item < 0)
			item = (menu->itemCount - 2);
			
		SetMenuSelect(activatedObject->string, item);
		DrawComboBox(selectedWindow, activatedObject);
	}
}

void SetActivNext(short isTab)
{
	sElement *currentElement = (sElement *)selectedWindow->first;
	sGraficObject *currentObject = (sGraficObject *)currentElement->data;

	short i    = 0;
	short bool = 0;

	for (i = 0; i < selectedWindow->itemCount; i++)
	{
		if (bool == 1)
		{
			if (currentObject->type == TA_EDIT_FIELD)
			{
				break;
			}
			else if (currentObject->type == TA_COMBO_BOX && isTab)
			{
				if (GetMenuFlag((void *)currentObject->string) != COMBO_BOX_DISABLED)
				{
					selectedEditField = activatedObject;
					break;
				}
			}
		}
		else if (currentObject == activatedObject)
		{
			bool = 1;
		}

		currentElement = (void *)currentElement->next;
		currentObject  = currentElement != 0 ? (sGraficObject *)currentElement->data : 0;

		if ((i + 1) == selectedWindow->itemCount)
		{
			i    = 0;
			bool = 1;

			currentElement = (sElement *)selectedWindow->first;
			currentObject  = (sGraficObject *)currentElement->data;
		}
	}

	ActivateEditField(selectedWindow, currentObject);
}

void SetActivPrev()
{
	sElement *currentElement = (sElement *)selectedWindow->first;
	sGraficObject *currentObject = (sGraficObject *)currentElement->data;
	sGraficObject *prevObject = NULL;
	sGraficObject *lastObject = NULL;

	short position = -1;
	short i;

	for (i = 0; i < selectedWindow->itemCount; i++)
	{
		if (currentObject == activatedObject)
			position = i;

		else if (position < 0 && currentObject->type == TA_EDIT_FIELD)
			prevObject = currentObject;

		else if (position >= 0 && currentObject->type == TA_EDIT_FIELD)
			lastObject = currentObject;

		currentElement = (void *)currentElement->next;
		currentObject = currentElement != 0 ? (sGraficObject *)currentElement->data : 0;

		if (selectedWindow->itemCount == (i + 1))
		{
			if (prevObject != NULL)
				currentObject = prevObject;
 
			else
				currentObject = lastObject; 

			break;
		}
	}

	ActivateEditField(selectedWindow, currentObject);
}
