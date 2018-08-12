#ifndef HELPER_H
#define HELPER_H

#include "sparse.h"

Sparse_half** create_adjacency(int N, int rows, 
		               Sparse_list *dataset, int **row_sums);
int count_elements(int *rows, Sparse_list *array);
double norm(double *a, double *b, int N);
void save_results(double *x, int N);
void validate(double *x, int N, char *filename);
double now();
double elapsed_time(double start, double end);
void normalize(double **x_init, int rows);
void err_exit(char *arg);

#endif
