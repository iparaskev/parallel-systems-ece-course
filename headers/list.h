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
    int size;
}list;

void append(list *l, int value);
void del(list *l, int value);
int get(list l, int index);

#endif

