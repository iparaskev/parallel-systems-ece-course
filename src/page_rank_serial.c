#include "data_parser.h"
#include "constants.h"
#include "helper.h"
#include <stdlib.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

double* pagerank(Sparse_half **adjacency, int rows, int *row_sums);

int 
main(int argc, char **argv)
{
	int N, rows, *row_sums; 
	double *R;
	char *dataset_path = argv[1];

	/* Read the dataset and count the nodes.*/
	Sparse_list *array = parse_data(dataset_path);	
	N = count_elements(&rows, array);
	printf("%d %d\n", N, rows);
	Sparse_half **adjacency = create_adjacency(N, rows, array, &row_sums);
	R = pagerank(adjacency, rows, row_sums);

	return 0;
}

double*
gs_mult(Sparse_half **A, double **x, double *b, int rows, int *row_sums)
{
	double *x_new, *x_old;
	x_new = *x;
	x_old = malloc(rows * sizeof *x_old);
	for (int i = 0; i < rows; i++)
		x_old[i] = x_new[i];
	double a_ii, value, x_i;
	int real_col;
	for (int row = 0; row < rows; row++)
	{
		a_ii = 1.;
		x_i = b[row];

		for (int column = 0; column < row_sums[row]; column++)
		{
			/* Extract the values from the sparse*/
			real_col = A[row][column].col;
			value = A[row][column].value;

			/* Add only the element on the diagonial.*/
			a_ii += value * (real_col == row);

			/* Update the vector */
			if (real_col < row)
				x_i -= value * x_new[real_col];
			else if (real_col > row)
				x_i -= value * x_new[real_col];
		}
		x_new[row] = x_i / a_ii;
	}
	
	return x_old;
}


double*
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
		delta = norm(x, x_old, rows);
		printf("iteration: %d delta: %0.15f\n", iterations, delta);
		iterations++;
		free(x_old);
	}
	double sumo = 0;
	for (int i = 0; i < rows; i++)
	{
		sumo += fabs(x[i]);
		
	}
	double pos = 0;
	for (int i = 0; i < rows; i++)
	{
		x[i] /= sumo;
		pos += x[i];
	}
	printf("Final %f\n", pos);

	return x;
}

