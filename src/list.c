#include <stdio.h>
#include <stdlib.h>
#include "list.h"

void
append(list *l, int value)
{

	/* See if it is the first*/
	if (l->tail == NULL)
	{
		l->element.value = value;
		l->element.next = NULL;
		l->tail = &(l->element);
	}
	else
	{
		/* Create new element*/
		linked_list *new;
		new = malloc(sizeof *new);

		/* Give the value.*/
		new->value = value;
		new->next = NULL;

		/* Add to the list*/
		l->tail->next = new;
		l->tail = new;
	}
}

void
remove(list *l, int value)
{
	
	linked_list *next; // The variable for deleting the node.

	/* Check the rest elements.*/
	while (l->head->next != l->tail)
	{
		if (l->head->next->value == value)
		{
			next = l->head->next;		
			l->head->next = next->next;
			free(next);
		}
		else
			l->head = l->head->next; // Go to the next node
	}

	/* Check the last element.*/
	if (l->head->next->value == value)
	{
		next = l->head->next;		
		l->head->next = NULL;
		l->tail = l->head;
		free(next);
	}

	l->head = &l->element;

	/* Check the first element.*/
	if (l->head->value == value)
	{
		l->head->value = l->head->next->value;
		l->head->next = l->head->next->next;
	}

}

//int 
//main(int argc, char **argv)
//{
//	list l;
//	l.head = &l.element;
//	l.tail = NULL;
//
//	for (int i = 0; i < 10; i++)
//		if (i == 5)
//			append(&l, i - 1);
//		else
//			append(&l, i );
//
//	linked_list *current = &l.element;
//	while (current != NULL)
//	{
//		printf("%d \n", current->value);
//		current = current->next;
//	}
//	puts("after");
//	delete(&l, 4);
//	delete(&l, 0);
//	delete(&l, 9);
//	current = l.head;
//	while (current != NULL)
//	{
//		printf("%d \n", current->value);
//		current = current->next;
//	}
//	return 0;
//}
