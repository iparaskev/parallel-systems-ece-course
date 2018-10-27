#ifndef HELPER_H
#define HELPER_H

void get_rows(char *name);
void get_columns(char *name);
double* load_examples(char *name);
double* load_labels(char *name);
int* init_neighbors(void);
double** init_neighbors_dist(void);
int check_labels(int *all_neighbors, double *labels);
void exchange(int* neighbors, double **neighbors_dist, 
	          int index, double dist,
	          int i, int j);
double norm(double *r);

#endif
