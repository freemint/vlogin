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



void PushBack(sList *list, void *data)

{

	sElement *element;

	

	if (list == 0)

		return;

		

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



void EraseFirst(sList *list)

{

	sElement *element;

	

	if (list == 0)

		return;

		

	element = list->first;

		

	if (list->first->next)

		list->first = (void *)list->first->next;

		

	list->itemCount--;

	

	free(element);

}