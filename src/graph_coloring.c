#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "data_parser.h"
#include "helper.h"
#include "list.h"

list* make_undirected(int rows, Sparse_list *data);
void coloring(list *graph, int rows);

int 
main(int argc, char **argv)
{
	int N, rows, *row_sums, *connections;
	Sparse_list *data = parse_data(argv[1]);
	Sparse_list *a = data;
	//while(a != NULL)
	//{
	//	printf("row %d, col %d, value %d\n", a->cell.row, a->cell.col, a->cell.value);
	//	a = a->next;
	//}
	N = count_elements(&rows, data);
	printf("N %d rows %d\n", N, rows);
	//Sparse_half **adjacency = create_adjacency(N, rows, data, &row_sums);
	list *undirected = make_undirected(rows, data);
	//und(adjacency, rows, row_sums);
	//for (int i = 0; i < rows; i++)
	//{
	//	printf("size %d %d	", undirected[i].size, i);
	//	linked_list *row = undirected[i].head;
	//	while (row != NULL)
	//	{
	//		printf("%d ", row->value);
	//		row = row->next;
	//	}
	//	printf("\n");
	//}
	coloring(undirected, rows);

	return 0;
}


void 
coloring(list *graph, int rows)
{
	srand(1300);
	list *P;     // The palletes of colors for every vertex.
	if ((P = malloc(rows * sizeof *P)) == NULL)
		err_exit("Malloc");
	
	list U;           // The uncolored vertexes.
	U.size = 0;
	U.tail = NULL;
	list I;          // The colored vertexes.
	I.size = 0;
	I.tail = NULL;
	int *color; // The color of assigned to every vertex.
	if ((color = malloc(rows * sizeof *color)) == NULL)
		err_exit("Malloc");
	
	int *next_color; // The next color of a vertex in case the pallete
			      // is empty
	if ((next_color = malloc(rows * sizeof *next_color)) == NULL)
		err_exit("Malloc");
	
	/* Find the biggest degree of the graph*/
	int max = -1;
	for (int row = 0; row <rows; row++)
		if (graph[row].size > max)
			max = graph[row].size;

	int max_colors = max / 3;
	max_colors = 4;

	/* Initialize palletes and uncolored vertexes*/
	for (int row = 0; row < rows; row++)
	{
		next_color[row] = max_colors + 1;
		append(&U, row);
		P[row].size = 0;
		P[row].tail = NULL;
		P[row].head = NULL;
		for (int color = 0; color < max_colors; color++)
			append(&P[row], color);
	}

	linked_list *a;
	a = U.head;

	int empty = 0;
	int idle = 0;
	linked_list *neighbor;
	while (!empty)
	{
		//puts("round");
		/* Assign random color to every vertex.*/
		neighbor = U.head;
		while (neighbor != NULL)
		{
			int vertex = neighbor->value;
			int index = rand() % P[vertex].size;
			color[vertex] = get(P[vertex], index);
			neighbor = neighbor->next;
		}
		
		/* Check if a vertex has unique color*/
		linked_list *vertex = U.head;
		while (vertex != NULL)
		{
			int row = vertex->value;
//			if (row %10000 == 0)
//				printf("row %d\n", row);
//
			//for (int i = 0; i < P[row].size; i++)
			//{
			//	linked_list *b = P[row].head;
			//	while(b != NULL)
			//	{
			//		printf("color %d \n", b->value);
			//		b = b->next;
			//	}
			//}
			neighbor = graph[row].head;
			int different = 1;
			while (neighbor != NULL)
			{
				if (color[neighbor->value] == color[row])
				{
					different = 0;
					break;
				}
				neighbor = neighbor->next;
			}

			/* For unique color update U, I and remove color from
			 * his neighbors.
			 */
			vertex = vertex->next;
			if (different)
			{
				neighbor = graph[row].head;
				/* Delete color from palletes*/
				for (; neighbor != NULL; neighbor = neighbor->next)
					del(&P[neighbor->value], color[row]);

				del(&U, row);
				append(&I, row);
			}

			/* Feed the hungry*/
			if (!P[row].size)
			{
				puts("o");
				append(&P[row], next_color[row]);
				next_color[row]++;	
			}
			//printf("Row %d Color %d\n", row, color[row]);
			//linked_list *a = P[row].head;
			//while (a != NULL)
			//{
			//	printf("%d\n", a->value);
			//	a = a->next;
			//}
		}
		printf("U size %d, I size %d\n", U.size, I.size);
		
		//linked_list *a = I.head;
		//while (a != NULL)
		//{
		//	printf("vert %d color %d\n", a->value, color[a->value]);
		//	a = a->next;
		//}

		if (I.size == 6)
			idle++;
		//if (idle > 2)
		//	break;
		if (I.size == rows)
			empty = 1;
	}
}

list*
make_undirected(int rows, Sparse_list *data)
{
	Sparse_list *current = data;

	/* An array with lists with the indexes of every connected node */
	list *undirected = malloc(rows * sizeof *undirected);
	if (undirected == NULL)
		err_exit("Malloc");

	/* Inititialize the lists.*/
	for (int i = 0; i < rows; i++)
	{
		undirected[i].head = &undirected[i].element;
		undirected[i].element.value = -1;
		undirected[i].size = 0;
		undirected[i].tail = NULL;
		undirected[i].head = NULL;
	}
	
	/* Find the size of every row on the undirected graph */
	while (current != NULL)
	{
		int row_index = current->cell.row - 1;
		int col_index = current->cell.col - 1;
		current = current->next;
		append(&undirected[row_index], col_index);
		append(&undirected[col_index], row_index);
	}

	return undirected;
}
