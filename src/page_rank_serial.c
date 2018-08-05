#include "data_parser.h"
#include "constants.h"
#include "helper.h"
#include "page_rank_serial.h"
#include <stdlib.h>
#include <stdio.h>
#include <omp.h>

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
			x_i -= value * x_new[real_col] * (real_col != row);
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
	puts("ok pa");
	double *b, *x, *x_old;
	b = malloc(rows * sizeof *x);
	if ((x = malloc(rows * sizeof *x)) == NULL)
	{
		perror("Malloc at x");
		exit(1);
	}
	printf("rows %d\n", rows);
	for (int i = 0; i < rows; i++)
	{
		b[i] = (1 - D) / rows;
		x[i] = 1. / rows;
	}

	int iterations = 0;
	double delta = 1;
	double t, t_s;
	while (iterations < MAX_ITERATIONS)
	{
		t = now();
		x_old = gs_mult(adjacency, &x, b, rows, row_sums);	
		t_s = now() - t;
		delta = norm(x, x_old, rows);
		printf("iteration: %d delta: %0.15f time: %0.15f\n", iterations, delta, t_s);
		iterations++;
		free(x_old);
	}
	normalize(&x, rows);
	printf("Iterations %d delta %.15f\n", iterations-1, delta);

	/* Clean up */
	free(b);
	return x;
}

