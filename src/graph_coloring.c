#include <stdio.h>
#include <stdlib.h>
#include "quick_sort.h"
#include "graph_coloring.h"
#include "globals.h"
#include "helper.h"

Sparse_half**
partitions(Sparse_half **A, int *color, list *borders)
{

	/* Make an array with the sorted indexes to use it after*/
	indexes = malloc(rows * sizeof *indexes);
	if (indexes == NULL)
		err_exit("Malloc");
	for (int i = 0; i < rows; i++)
		indexes[i] = i;

	/* Sort the color array and change the indexes to partition the array*/
	quick_sort(color, indexes, 0, rows - 1);

	/* An array that has the new index for every vertex*/
	int *map = malloc(rows * sizeof *map);
	if (map == NULL)
		err_exit("Malloc");
	for (int row = 0; row < rows; row++)
		map[indexes[row]] = row; 
		
	/* The new partitioned adjacency matrix*/
	Sparse_half **A_new = malloc(rows * sizeof *A_new);
	if (A_new == NULL)
		err_exit("Malloc");

	/* The new row sums array.*/
	int *elements = malloc(rows * sizeof *elements);
	if (elements == NULL)
		err_exit("Malloc");

	for (int row = 0; row < rows; row++)
	{
		A_new[row] = A[indexes[row]];
		elements[row] = row_sums[indexes[row]];
		/* Update the indexes inside the row*/
		for (int col = 0; col < row_sums[indexes[row]]; col++)
			A_new[row][col].col = map[A_new[row][col].col];
	}
	
	/* Clean up*/
	free(row_sums);
	free(map);

	find_borders(borders, color);
	row_sums = elements;
	return A_new;
}

int* 
coloring(list *graph)
{
	/* The max colors is the maximum degree of the graph.*/
	int max_colors = -1;
	for (int row = 0; row < rows; row++)
		if (graph[row].size > max_colors)
			max_colors = graph[row].size;
	max_colors++;
	max_colors = 400;

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
	double t = now();
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

	printf("g %f \n", now() - t);
	//exit(1);
	return color;
}

list*
make_undirected(Sparse_list *data)
{
	Sparse_list *current = data;

	/* An array with lists with the indexes of every connected node */
	list *undirected = malloc(rows * sizeof *undirected);
	if (undirected == NULL)
		err_exit("Malloc");

	/* Inititialize the lists.*/
	for (int i = 0; i < rows; i++)
	{
		undirected[i].element.value = -1;
		undirected[i].size = 0;
		undirected[i].tail = NULL;
		undirected[i].head = NULL;
	}
	
	while (current != NULL)
	{
		int row_index = current->cell.row - 1;
		int col_index = current->cell.col - 1;
		current = current->next;
		append(&undirected[col_index], row_index);
		append(&undirected[row_index], col_index);
	}

	return undirected;
}

void
find_borders(list *borders, int *color)
{
	borders->head = NULL;
	borders->tail = NULL;
	borders->size = 0;

	int current = color[0];
	for (int row = 1; row < rows; row++)
		if (color[row] != current)
		{
			append(borders, row);
			current = color[row];
		}
}
