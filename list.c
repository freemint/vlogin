#include "list.h"


sList *CreateList()
{
	sList *list = (sList *)malloc(sizeof(sList));

	list->itemCount = 0;
	list->first = NULL;
	list->last = NULL;

	return list;
}



void DestroyList(sList *list)
{
	free(list);
}

void PushFront(sList *list, void *data)
{
	sElement *element;

	if (!list) return;

	element = (sElement *)malloc(sizeof(sElement));
	element->data = data;
	element->next = (void *)list->first;

	list->first = element;

	if (!list->last)
		list->last = (void *)element;

	list->itemCount++;
}

void PushBack(sList *list, void *data)
{
	sElement *element;

	if (!list) return;

	element = (sElement *)malloc(sizeof(sElement));
	element->data = data;
	element->next = NULL;

	if (list->last)
		list->last->next = (void *)element;
	else
		list->first = element;

	list->last = element;
	list->itemCount++;
}

void EraseFront(sList *list)
{
	sElement *element;

	if (!list) return;

	element = list->first;

	if (list->first->next)
	{
		list->first = (void *)list->first->next;
	}
	else
	{
		list->first = NULL;
		list->last  = NULL;
	}

	list->itemCount--;

	free(element);
}

void EraseIndex(sList *list, int index)
{
	sElement *element;
	void *previousElement;
	int i;

	if (!list) return;
	if (list->itemCount > index) return;

	element = list->first;

	for (i = 0; i < index - 1; i ++)
		element = (sElement *)element->next;

	previousElement = element->next;
	element->next = (void *)((sElement *)element->next)->next;

	list->itemCount--;

	free(previousElement);
}
