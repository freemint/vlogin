#ifndef _LIST_H_
#define _LIST_H_


#include <memory.h>
#include <stdio.h>


typedef struct
{
	struct sElement *next;
	void *data;
} sElement;

typedef struct
{
	sElement *first;
	sElement *last;
	short itemCount;
} sList;


sList *CreateList();
void DestroyList(sList *list);

void PushFront(sList *list, void *data);
void PushBack(sList *list, void *data);
void EraseFront(sList *list);
void EraseIndex(sList *list, int index);

#endif

