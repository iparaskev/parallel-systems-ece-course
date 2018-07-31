#ifndef HELPER_H
#define HELPER_H

#include "sparse.h"

Sparse_half** create_adjacency(int N, int rows, 
		               Sparse_list *dataset, int **row_sums);
int count_elements(int *rows, Sparse_list *array);
double norm(double *a, double *b, int N);

#endif
