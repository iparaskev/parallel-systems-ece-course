#ifndef KNN_SOLUTION_H
#define KNN_SOLUTION_H

int* find_kneighbors_serial(double *array);
int* find_kneighbors_blocking(double *array);
int* find_kneighbors_nonblocking(double *array);
int translate_index(int mes, int j);
void n_norm(double *norms, double *array, int start, int N, int find_end);
void distances(
               double **dists, double *array, 
	           double *comp_array, int size_comp,
	           double *norms_rows, double *norms_cols,
	           int i, int j,
	           int N);
void update_distances(int *neighbors, double **neighbors_dist,
                      double **dists, double *array, 
	                  double *comp_array, int size_comp,
	                  double *norms_rows, double *norms_cols,
	                  int i, int j,
	                  int N, int mes);




#endif
