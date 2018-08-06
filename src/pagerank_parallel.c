#include <stdlib.h>
#include <stdio.h>
#include "helper.h"
#include "constants.h"
#include <pthread.h>

#define NUM_THREADS 1

/* Global variables */
double *x_new;        // The pagerank vector.
double a_ii;          // Variable for division at Gauss Seidel.
double x_i;           // The variable used for updating a row in the vector.
int row_elements;     // The number of elements of the current row.
int row_id;           // The current row index.
int N;		      // The number of rows.
Sparse_half *row_A;   // The current row.
int elements_per_thr; // How many elements any thread will have.
int done;             // A counter for the finished threads.
int ready;            // A counter for the threads that are ready to start.

/* Mutexes and condition variables.*/
static pthread_mutex_t mutex_start = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t mutex_done = PTHREAD_MUTEX_INITIALIZER;

static pthread_cond_t cond_main = PTHREAD_COND_INITIALIZER;
static pthread_cond_t cond_thr = PTHREAD_COND_INITIALIZER;

/* Function to print error and exit the process for visual reasons */
void 
err_exit(char *arg)
{
	fprintf(stderr, "%s\n", arg);
	exit(1);
}

void*
compute_row(void *args)
{

	int id = (long) args;
	int s;
	/*inside loop variables*/
	double value;
	int real_col, start, end;
	for(;;)
	{
		/* Put thread to sleep until main is ready.*/
		s = pthread_mutex_lock(&mutex_start);
		if (s != 0)
			err_exit("Error at mutex lock");

		ready++;
		//printf("thread bef %d %d\n", id, ready);
		
		s = pthread_cond_wait(&cond_thr, &mutex_start);
		if (s != 0)
			err_exit("Error at mutex lock");
		//printf("thread after %d %d\n", id, ready);
		
		s = pthread_mutex_unlock(&mutex_start);
		if (s != 0)
			err_exit("Error at mutex unlock");

		
		/* Compute the local sum of elements.*/
		start = id * elements_per_thr;
		if (id == NUM_THREADS - 1)
			end = row_elements;
		else
			end = start + elements_per_thr;

		double local_sum = 0;
		double local_ai = 0;
		for (int i = start; i < end; i++)
		{
			/* Get the real values.*/
			real_col = row_A[i].col;
			value = row_A[i].value;
			
			local_ai += value * (real_col == row_id);

			/* Update the sum.*/
			local_sum += value \
				     * x_new[real_col] \
				     * (real_col != row_id);
		}

		/* Update global x_i, a_i and done variables.*/
		s = pthread_mutex_lock(&mutex_done);
		if (s != 0)
			err_exit("Error at mutex lock");

		x_i -= local_sum;
		a_ii += local_ai;
		done++;

		/* Wake up main thread to update the vector.*/
		if (done == NUM_THREADS)
			s = pthread_cond_signal(&cond_main);

		s = pthread_mutex_unlock(&mutex_done);
		if (s != 0)
			err_exit("Error at mutex unlock");

		if (row_id == (N - 1))
			break;
	}
	
	return NULL;
}

double*
gs_mult_par(Sparse_half **A, double **x, double *b, int rows, int *row_sums)
{
	x_new = *x;
	double *x_old;
	x_old = malloc(rows * sizeof *x_old);
	for (int i = 0; i < rows; i++)
		x_old[i] = x_new[i];

	int s; // Variable for return values of thread functions

	/* Initialize threads.*/
	pthread_t threads[NUM_THREADS];

	/* Threads call*/
	ready = 0;
	for (int i = 0; i < NUM_THREADS; i++)
	{
		pthread_create(&threads[i], NULL, compute_row, (void *) (long) i);
		if (s != 0)
			err_exit("pthread create.");
	}

	for (int row = 0; row < rows; row++)
	{
		//printf("row %d\n", row);
		a_ii = 1.;
		row_id = row;
		x_i = b[row];
		row_A = A[row];
		row_elements = row_sums[row];
		elements_per_thr = row_elements / NUM_THREADS;

		/* Wake threads to compute the element.*/
		for (;;)
		{
			//puts("1");
			s = pthread_mutex_lock(&mutex_start);
			if (ready == NUM_THREADS)
			{
				ready = 0;
				s = pthread_cond_broadcast(&cond_thr);
				if (s != 0)
					err_exit("pthread condition broadcast.");
				s = pthread_mutex_unlock(&mutex_start);
				break;
			}
			s = pthread_mutex_unlock(&mutex_start);
		}
		//puts("2");

		/* Go to sleep until the other threads finish their job.*/
		s = pthread_mutex_lock(&mutex_done);
		if (s != 0)
			err_exit("pthread mutex lock.");
		while (done != NUM_THREADS)
		{
			s = pthread_cond_wait(&cond_main, &mutex_done);
			if (s != 0)
				err_exit("pthread condition wait.");
		}
		done = 0;
		s = pthread_mutex_unlock(&mutex_done);
		if (s != 0)
			err_exit("pthread mutex unlock.");

		/* Update element.*/
		x_new[row] = x_i / a_ii;
		//puts("3");
	
		
	}

	for (int i = 0; i < NUM_THREADS; i++)
		pthread_join(threads[i], NULL);
	
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
	N = rows;
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
	while ( delta > TOL && iterations < MAX_ITERATIONS)
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

