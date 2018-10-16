#include "globals.h"

int rows, columns;
int k;
int el_per_proc;
int files;
int rank;
int world_size;
int *neighbors;
int *all_neighbors;
double *array;
double **neighbors_dist;
double *labels;
