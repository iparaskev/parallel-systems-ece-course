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
		l->size++;
		l->element.next = NULL;
		l->tail = &(l->element);
		l->head = &(l->element);
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
		l->size++;
	}
}

void
del(list *l, int value)
{
	
	linked_list *next; // The variable for deleting the node.

	/* Check the rest elements.*/
	if (l->size > 2)
		while (l->head->next != l->tail)
			if (l->head->next->value == value)
			{
				next = l->head->next;		
				l->head->next = next->next;
				l->size--;
				free(next);
			}
			else
				l->head = l->head->next; // Go to the next node

	/* Check the last element.*/
	if (l->size > 1)
		if (l->head->next->value == value)
		{
			next = l->head->next;		
			l->head->next = NULL;
			l->tail = l->head;
			l->size--;
			free(next);
		}

	l->head = &l->element;

	/* Check the first element.*/
	if (l->head->value == value)
	{
		l->size--;
		if (l->size == 0)
		{
			l->head->value = -1;
			l->head->next = NULL;
			l->tail = NULL;
		}
		else
		{
			l->head->value = l->head->next->value;
			l->head->next = l->head->next->next;
		}
	}

}

int
get(list l, int index)
{
	int counter = 0;
	int return_value;
	while (l.head != NULL)
	{
		if (counter == index)
		{
			return_value = l.head->value;
			break;
		}
		counter++;
		l.head = l.head->next;
	}

	if (counter == l.size)
		return -1;
	else
		return return_value;
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
//	printf("found %d\n", get(l, 3));
//	del(&l, 4);
//	del(&l, 0);
//	del(&l, 9);
//	printf("found %d\n", get(l, 3));
//	current = l.head;
//	while (current != NULL)
//	{
//		printf("%d \n", current->value);
//		current = current->next;
//	}
//	current = l.head;
//	for (; current != NULL; current = current->next)
//		printf("%d\n", current->value);
//	return 0;
//}
