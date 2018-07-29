#include "data_parser.h"
#include <stdlib.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

typedef struct mid_cell
{
	int col;
	float value;
} Sparse_half;
Sparse_half** create_adjacency(int N, int rows, 
		               Sparse_list *dataset, int **row_sums);
int count_elements(int *rows, Sparse_list *array);

int 
main(int argc, char **argv)
{
	int N, rows, *row_sums; 
	char *dataset_path = argv[1];

	/* Read the dataset and count the nodes.*/
	Sparse_list *array = parse_data(dataset_path);	
	N = count_elements(&rows, array);
	printf("%d %d\n", N, rows);
	Sparse_half **adjacency = create_adjacency(N, rows, array, &row_sums);
	//for (int i = 0; i < rows; i++)
	//	for (int j = 0; j < row_sums[i]; j++)
	//		printf("%d %d %f\n", i, adjacency[i][j].col, adjacency[i][j].value);

	return 0;
}

/* Count the elements of the sparse dataset and find the number of unique nodes.
 *
 * Arguments:
 * rows -- the variable where the last index will be saved
 * array -- the linked list with the sparse array
 *
 * Output:
 * N -- the number of sparse cells
 */
int 
count_elements(int *rows, Sparse_list *array)
{
	int last_index = -1;
	int N = 0;
	while (array != NULL){
		if (array->cell.row > last_index)
			last_index = array->cell.row;
		if (array->cell.col > last_index)
			last_index = array->cell.col;
		array = array->next;
		N++;
	}
	*rows = last_index;
	
	return N;
}

/* Append an element to the end of the linked list and make the new element the
 * new current element.
 *
 * Arguments:
 * cell -- the new sparse cell of the list
 * current -- the last element before the change
 */
void 
append(Sparse cell, Sparse_list **current)
{
	Sparse_list *next, *end;
	end = *current;
	if ((next = malloc(sizeof *next)) == NULL)
	{
		perror("Malloc at appending element");
		exit(1);
	}

	next->cell = cell;
	next->next = NULL;
	end->next = next;
	end = end->next;
	*current  = end;
}

/* Computes the complete adjacency matrix from the sparse list, also it fills
 * the rows with all zeros with a normal distribuded row.
 *
 * Arguments:
 * N -- the number of the cells of the initial sparse matrix
 * rows -- the number of rows of the sparse matrix
 * dataset -- a pointer to the start of the linked list that contains the 
 *            initial sparse
 *
 * Output:
 * The final sparse array.
 */ 
Sparse_half**
create_adjacency(int N, int rows, Sparse_list *dataset, int **row_elements)
{
	/* Find the total elements of every row of sparse matrix.*/
	int *row_sums;  // An array for the total elements of every row.
	if ((row_sums = malloc(rows * sizeof *row_sums)) == NULL)
	{
		perror("Malloc at creation of row_sums");
		exit(1);
	}
	memset(row_sums, 0, rows); // Initialization of the sums array.

	/* Access all the elements of the list to count the elements per row.*/
	Sparse_list *current = dataset;
	while (current != NULL)
	{
		row_sums[current->cell.row - 1]++;
		current = current->next;
	}

	/* A row with normal distributed elements */
	Sparse_half *normal;
	if ((normal = malloc(rows * sizeof *normal)) == NULL)
	{
		perror("Malloc at normal row");
		exit(1);
	}
	for (int i = 0; i < rows; i++)
	{
		normal[i].col = i;
		normal[i].value = 1. / rows;
	}

	/* Create the known sparse array.*/
	Sparse_half **adjacency;
	if ((adjacency = malloc(rows * sizeof *adjacency)) == NULL)
	{
		perror("Malloc at creation of adjacency");
		exit(1);
	}
	/* Initialize the rows*/
	for (int i = 0; i < rows; i++)
	{
		if (row_sums[i])
		{
			adjacency[i] = malloc(row_sums[i] * sizeof **adjacency);
			if (adjacency[i] == NULL)
			{
				perror("Malloc at creation of adjacency");
				exit(1);
			}
		}
		else
		{
			adjacency[i] = normal;
			row_sums[i] = rows;
		}
	}

	/* An array for tracking at which element for every row we are*/
	int *indexes;
	if ((indexes = malloc(rows * sizeof *indexes)) == NULL)
	{
		perror("Malloc");
		exit(1);
	}
	memset(indexes, 0, rows);

	current = dataset;
	int row, column;
	while (current != NULL)
	{
		row = current->cell.row - 1;
		column = indexes[row];
		indexes[row]++;
		adjacency[row][column].col = current->cell.col - 1;
		adjacency[row][column].value = (float) 1 / row_sums[row];
		current = current->next;
	}
	
	*row_elements = row_sums;
	return adjacency;
}
