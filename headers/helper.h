#include "sparse.h"

#ifndef HELPER_H
#define HELPER_H

Sparse_half** create_adjacency(int N, Sparse_list *dataset);
int count_elements(Sparse_list *array);
double norm(double *a, double *b, int N);
void save_results(double *x, int N);
void validate(double *x, int N, char *filename);
double now();
double elapsed_time(double start, double end);
void normalize(double **x_init);
void err_exit(char *arg);

#endif
