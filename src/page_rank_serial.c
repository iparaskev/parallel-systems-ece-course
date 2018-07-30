#include "data_parser.h"
#include <stdlib.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

typedef struct mid_cell
{
	int col;
	float value;
} Sparse_half;
Sparse_half** create_adjacency(int N, int rows, 
		               Sparse_list *dataset, int **row_sums);
int count_elements(int *rows, Sparse_list *array);
void append(Sparse cell, Sparse_list **current);
void pagerank(Sparse_half **adjacency, int rows, int *row_sums);

/* Define some things to go later to a spare file*/
#define TOL 1e-7
#define MAX_ITERATIONS 50
#define D 0.85


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
	pagerank(adjacency, rows, row_sums);

	return 0;
}

/*TODO make b value and not an array*/
double*
gs_mult(Sparse_half **A, double **x, double *b, int rows, int *row_sums)
{
	double *x_new, *x_old;
	x_new = malloc(rows * sizeof *x_new);
	x_old = *x;
	double a_ii, value;
	int real_col;
	for (int row = 0; row < rows; row++)
	{
		a_ii = 1.;
		x_new[row] = b[row];

		for (int column = 0; column < row_sums[row]; column++)
		{
			/* Extract the values from the sparse*/
			real_col = A[row][column].col;
			value = A[row][column].value;

			/* Add only the element on the diagonial.*/
			a_ii += value * (real_col == row);

			/* Update the vector */
			if (real_col < row)
				x_new[row] -= value * x_new[real_col];
			else if (real_col > row)
				x_new[row] -= value * x_old[real_col];
		}
		x_new[row] /= a_ii;
		//if (row == 9658)
		//	printf("Value %f\n", x_new[row]);
	}
	
	*x = x_new;
	return x_old;
}

double
norm(double *a, double *b, int N)
{
	double sumo = 0;
	for (int i = 0; i < N; i++)
	{
		sumo += fabs(a[i] - b[i]);
		//if (i < 9659 && i > 9657)
		//	printf("%f %f %f\n", a[i], b[i], sumo);
	}

	//printf("norm %f\n", sumo);
	return sumo;
}

void
pagerank(Sparse_half **adjacency, int rows, int *row_sums)
{
	/* Gauss seidel solves the Ax=b equation and the algebraic version of
	 * pagerank is R = (I - dM)^(-1) * (1-d/N). So for our problem at Gauss
	 * Seidel we 've got A = (I + adjacency), b = (1-d/N)vector. 
	 */

	/* Initialize b vector and initial pagerank*/
	double b[rows], *x, *x_old;
	if ((x = malloc(rows * sizeof *x)) == NULL)
	{
		perror("Malloc at x");
		exit(1);
	}
	if ((x_old = malloc(rows * sizeof *x_old)) == NULL)
	{
		perror("Malloc at x_old");
		exit(1);
	}
	for (int i = 0; i < rows; i++)
	{
		b[i] = (1 - D) / rows;
		x[i] = 1. / rows;
	}

	int iterations = 0;
	double delta = 1;
	while (delta > TOL && iterations < MAX_ITERATIONS)
	{
		x_old = gs_mult(adjacency, &x, b, rows, row_sums);	
		//for (int i = 9957; i < 9959; i++)
		//	printf("%f \n", x[i]);
		
		delta = norm(x, x_old, rows);
		printf("iteration: %d delta: %0.15f\n", iterations, delta);
		iterations++;
	}
	double sumo = 0;
	for (int i = 0; i < rows; i++)
	{
		sumo += fabs(x[i]);
		
	}
	for (int i = 0; i < rows; i++)
	{
		if (i < 10)
			printf("%f\n", x[i] / sumo);
	}
	printf("Final %f\n", sumo);
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

	int *col_sums; // An array with the total incoming elements.
	if ((col_sums = malloc(rows * sizeof *col_sums)) == NULL)
	{
		perror("Malloc at creation of col_sums");
		exit(1);
	}
	memset(col_sums, 0, rows); // Initialization of the sums array.

	/* Access all the elements of the list to count the elements per row.*/
	Sparse_list *current = dataset;
	while (current != NULL)
	{
		row_sums[current->cell.row - 1]++;
		col_sums[current->cell.col - 1]++;
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
		normal[i].value = -D * 1./rows;
	}

	/* Create the known sparse array.*/
	Sparse_half **adjacency;
	if ((adjacency = malloc(rows * sizeof *adjacency)) == NULL)
	{
		perror("Malloc at creation of adjacency");
		exit(1);
	}
	/* Initialize the rows and if one is all zeros assign the normal*/
	for (int i = 0; i < rows; i++)
	{
		adjacency[i] = malloc(col_sums[i] * sizeof **adjacency);
		if (adjacency[i] == NULL)
		{
			perror("Malloc at creation of adjacency");
			exit(1);
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

	/* Run through the list and for every cell assign the value to the 
	 * right row and column of the contigius adjacency matrix 
	 */
	current = dataset;
	int row, column;
	while (current != NULL)
	{
		row = current->cell.col - 1; 
		column = indexes[row];
		indexes[row]++;
		adjacency[row][column].col = current->cell.row - 1;
		adjacency[row][column].value = -D * 1./row_sums[current->cell.row - 1];
		current = current->next;
	}
	
	*row_elements = col_sums;
	return adjacency;
}
