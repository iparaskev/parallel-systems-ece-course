#ifndef LIST_H
#define LIST_H

typedef struct elements
{
	int value;
	struct elements *next;
} linked_list;

typedef struct lists
{
	linked_list *head, *tail;
	linked_list element;
}list;

void append(list *l, int value);
void remove(list *l, int value);

#endif

