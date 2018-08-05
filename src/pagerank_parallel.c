#include <stdlib.h>
#include <stdio.h>
#include "helper.h"
#include "constants.h"
#include <pthread.h>

/* Global constants */
double *x_new;
Sparse_half *row_A;
int row_id;

typedef struct thread_args
{
	int id;
	int N;
	double *a_i, *x_i;
} in_args;

void*
compute_row(void *args)
{
	in_args thread_args = *((in_args *) args);
	int start = thread_args.id * thread_args.N; // The start of iterations
	int end = start + thread_args.N; // The end of iterations
	double x_i = 0.;
	double a_i = *(thread_args.a_i);

	/*inside loop variables*/
	double value;
	int real_col;
	//printf("Thread %d N %d\n", thread_args.id, thread_args.N);
	for (int i = start; i < end; i++)
	{
		/* Get the real values.*/
		real_col = row_A[i].col;
		value = row_A[i].value;
		
		a_i += value * (real_col == row_id);

		/* Update the sum.*/
		x_i += value * x_new[real_col] * (real_col != row_id);
	}

	*(thread_args.a_i) = a_i;
	*(thread_args.x_i) = x_i;
	return NULL;
}

double*
gs_mult_par(Sparse_half **A, double **x, double *b, int rows, int *row_sums)
{
	double *x_old;
	x_new = *x;
	x_old = malloc(rows * sizeof *x_old);
	for (int i = 0; i < rows; i++)
		x_old[i] = x_new[i];

	double a_ii, value, x_i;
	int real_col;

	pthread_t *threads;  // The threads pointer to be used
	int s; // Variable for return values of thread functions
	int elementes_per_thr; // how many elements per thread
	in_args *input_args;
	double t;
	double init_sum = 0.;
	double calls_sum = 0.;
	double results_sum = 0.;

	for (int row = 0; row < rows; row++)
	{
		t = now();
		//printf("row %d N %d\n", row, row_sums[row]);
		a_ii = 1.;
		x_i = b[row];
		row_A = A[row];

		/* Dynamic number of threads to not burn resourcea */
		int num_threads = 4;
		threads = malloc(num_threads * sizeof *threads);
		if (threads == NULL)
		{
			perror("Malloc at threads initialization.");
			exit(0);
		}
		
		/* Dynamic creation of threads arguments */
		input_args = malloc(num_threads * sizeof *input_args);
		if (input_args == NULL)
		{
			perror("Malloc at threads initialization.");
			exit(0);
		}

		elementes_per_thr = row_sums[row] / num_threads;
		init_sum += now() - t;

		t = now();
		/* Call thread function.*/
		for (int i = 0; i < num_threads; i++)
		{
			/* Give values to every thread arguments */
			input_args[i].id = i;
			input_args[i].a_i = &a_ii;
			//input_args[i].x_i = malloc(sizeof *input_args[i].x_i);

		//	/* Handle the case when row_sums isn't divisible from
		//	 * num_threads.*/
		//	if (i == num_threads)
		//		input_args[i].N = row_sums[row] - i*elementes_per_thr;
		//	else
		//		input_args[i].N = elementes_per_thr;

		//	s = pthread_create(&threads[i], NULL,
		//		           compute_row, (void *) &input_args[i]);
		//	if (s != 0)
		//	{
		//		fprintf(stderr, "Error at thread creation.\n");
		//		exit(1);
		//	}
		}
		calls_sum += now() - t;

		/* Get thread results.*/
		t = now();
		//for (int i = 0; i < num_threads; i++)
		//{
		//	s = pthread_join(threads[i], NULL);
		//	if (s != 0)
		//	{
		//		fprintf(stderr, "Error at thread results.\n");
		//		exit(1);
		//	}
		//	x_i -= *(input_args[i].x_i);
		//	free(input_args[i].x_i);
		//}
		results_sum += now() - t;

		/* Clean up */
		free(threads);
		free(input_args);
		x_new[row] = x_i / a_ii;
	}
	printf("Init_time %f Calls_time %f Res_time %f\n", init_sum, calls_sum, results_sum);
	
	return x_old;
}


double*
pagerank_par(Sparse_half **adjacency, int rows, int *row_sums)
{
	/* Gauss seidel solves the Ax=b equation and the algebraic version of
	 * pagerank is R = (I - dM)^(-1) * (1-d/N). So for our problem at Gauss
	 * Seidel we 've got A = (I + adjacency), b = (1-d/N)vector. 
	 */

	/* Initialize b vector and initial pagerank*/
	double *b, *x, *x_old;
	b = malloc(rows * sizeof *b);
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
	double t, t_s;
	while ( iterations < MAX_ITERATIONS)
	{
		t = now();
		x_old = gs_mult_par(adjacency, &x, b, rows, row_sums);	
		t_s = now() - t;
		delta = norm(x, x_old, rows);
		printf("iteration: %d delta: %0.15f time: %0.15f\n", iterations, delta, t_s);
		iterations++;
		free(x_old);
	}
	normalize(&x, rows);
	printf("Iterations %d delta %.15f\n", iterations-1, delta);

	return x;
}

