#ifndef GLOBALS_H
#define GLOBALS_H

extern int rows, columns;
extern int k;
extern int el_per_proc;
extern int files;
extern int rank;
extern int world_size;
extern int *neighbors;
extern int *all_neighbors;
extern double *array;
extern double **neighbors_dist;
extern double *labels;

#endif
