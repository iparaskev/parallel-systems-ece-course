#include <stdlib.h>
#include <stdio.h>
#include "helper.h"
#include "constants.h"
#include "globals.h"
#include "list.h"
#include <omp.h>

/* Global variables*/
double *x_old;
double *x;

void
gs_mult_par(Sparse_half **A, double *b, int start, int end)
{
	
	double a_ii, value, x_i;
	int real_col;
	#pragma omp parallel for num_threads(num_thread)
	for (int row = start; row < end; row++)
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
			x_i -= value * x[real_col] * (real_col != row);
		}
		x[row] = x_i / a_ii;
	}
}


double*
pagerank_par(Sparse_half **adjacency, list *borders) 
{
	/* Gauss seidel solves the Ax=b equation and the algebraic version of
	 * pagerank is R = (I - dM)^(-1) * (1-d/N). So for our problem at Gauss
	 * Seidel we 've got A = (I + adjacency), b = (1-d/N)vector. 
	 */

	/* Initialize b vector and initial pagerank*/
	double *b;
	b = malloc(rows * sizeof *x);
	if ((x = malloc(rows * sizeof *x)) == NULL)
	{
		perror("Malloc at x");
		exit(1);
	}

	if ((x_old = malloc(rows * sizeof *x_old)) == NULL)
		err_exit("Malloc");

	for (int i = 0; i < rows; i++)
	{
		b[i] = (1 - D) / rows;
		x[i] = 1. / rows;
	}

	int iterations = 0;
	double delta = 1;
	double t, t_s;
	linked_list *current;
	while (delta > TOL && iterations < MAX_ITERATIONS)
	{
		t = now();
		current = borders->head;

		/* Copy old values */
		for (int i = 0; i < rows; i++)
			x_old[i] = x[i];

		int start = 0;
		/* Compute new values*/
		for (; current != NULL; current = current->next)
		{
			int end = current->value;
			gs_mult_par(adjacency, b, start, end);	
			start = end;
		}
		gs_mult_par(adjacency, b, start, rows);	

		t_s = now() - t;
		delta = norm(x, x_old, rows);
		//printf("iteration: %d delta: %0.15f time: %0.15f\n", iterations, delta, t_s);
		iterations++;
	}
	normalize(&x);
	printf("Iterations %d delta %.15f\n", iterations-1, delta);

	/* Clean up */
	free(b);
	return x;
}

