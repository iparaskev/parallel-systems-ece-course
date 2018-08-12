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
	/* The max colors is the maximum degree of the graph.*/
	int max_colors = -1;
	for (int row = 0; row < rows; row++)
		if (graph[row].size > max_colors)
			max_colors = graph[row].size;
	max_colors++;

	/* The array with the colors for every vertex.*/
	int *color = malloc(rows * sizeof *color);
	if (color == NULL)
		err_exit("Malloc");
	for (int i = 0; i < rows; i++)
		color[i] = -1;

	/* Array with the available colors for every node*/
	int *available = malloc(max_colors * sizeof *available);
	if (available == NULL)
		err_exit("Malloc");

	/* Assign the first color to the first vertex and start the procedure.*/
	color[0] = 0;
	for (int row = 1; row < rows; row++)
	{
		/* Make available all the colors*/
		for (int i = 0; i < max_colors; i++)
			available[i] = 1;

		/* Mark as unavailable the colors of the neighbors*/
		linked_list *vertex = graph[row].head;
		for (; vertex != NULL; vertex = vertex->next)
			if (color[vertex->value] != -1)
				available[color[vertex->value]] = 0;	

		/*Give the first available color*/
		for (int i = 0; i < max_colors; i++)
			if (available[i])
			{
				color[row] = i;
				break;
			}
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
