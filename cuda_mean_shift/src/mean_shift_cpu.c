#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <limits.h>
#include <sys/time.h>

// global variables
int rows, columns;

// local functions
double *read_data(char *name);
void get_exp_sparse(double *x, double *y, double h, double ***w_updated, int **counter_per_row);
void multiply_sparse(double *x, double **w, int *counters, double **y);
void get_sparse_sum_per_row(double **w, int *counters, double **sums);
double update_y(double *y_new, double *sums_array, double **y_updated);
int check_results(double *y, char *name);
void write_results(char **argv, int iterations, double time);

double
now()
{
 struct timeval tv;
 gettimeofday(&tv, 0);
 return tv.tv_sec + tv.tv_usec / 1000000.0;
}

int main(int argc, char **argv)
{
  double *x, *y, h, epsilon, *y_new, *sums_array;
  double **w_sparse;
  int *counter_per_row;

  if (argc < 6){
  	fprintf(stderr, "it needs 5 arguments, which are: h iterations data validation_data dims\n");
  	exit(1);
  }
  h = atoi(argv[1]);
  int iterations = atoi(argv[2]);

  // initialize arrays
	columns = atoi(argv[5]);
	x = read_data(argv[3]);
	y = malloc(sizeof *y * rows * columns);
	memcpy(y, x, sizeof *y * rows * columns);
	y_new = malloc(sizeof *y_new * rows * columns);
	sums_array = malloc(sizeof *sums_array * rows);
	w_sparse = malloc(sizeof *w_sparse * rows);
	counter_per_row = malloc(sizeof *counter_per_row * rows);

	double norm = INT_MAX;
	epsilon = 1e-4*h;
	double t_start = now(), t_end;
	
	double s;
	int j = 0;
	printf("Start %d\n", iterations);
	while (sqrt(norm) > epsilon){
	// for (int j = 0; j < iterations; j++){
		
		// get_exp(x, y, h, &w);
		get_exp_sparse(x, y, h, &w_sparse, &counter_per_row);
		
		multiply_sparse(x, w_sparse, counter_per_row, &y_new);
		
		// get_sum_per_row(w, &sums_array);
		get_sparse_sum_per_row(w_sparse, counter_per_row, &sums_array);

		// normalize and update mean shift
		norm = update_y(y_new, sums_array, &y);
		for(int k = 0; k < rows; k++)
			free(w_sparse[k]);
		printf("iteration %d error %f\n", j, sqrt(norm));
		j++;	
	}
	t_end = now() - t_start;
	printf("time passed %f\n", t_end);
	
	// write_results(argv, j, t_end);
	check_results(y, argv[4]);
	FILE *f = fopen("res.txt", "w");
	for (int i = 0; i < rows; i++){
		for (int j = 0; j < columns; j++)
			fprintf(f, "%f ", y[i * columns + j]);
		fprintf(f, "\n");
	}
	fclose(f);

	// clean up
	free(x);
	free(y);
	free(sums_array);

	return 0;
}

void 
multiply_sparse(double *x, double **w, int *counters, double **y)
{
	double *dot_array = *y;
	memset(dot_array, 0, sizeof *dot_array * rows * columns); 
	
	// compute dot product between x and w
	for (int i = 0; i < rows; i++){
		for(int j = 0; j < columns; j++)
			for (int k = 0; k < (2 * counters[i]); k += 2){
				int index = w[i][k];
				dot_array[i * columns + j] += w[i][k + 1] * x[index * columns + j];
			}
	}
	// free(*y);
	*y = dot_array;		
}

void 
get_sparse_sum_per_row(double **w, int *counters, double **sums)
{
	double *sums_array = *sums;
	//zero sums
	memset(sums_array, 0, sizeof *sums_array * rows);
	for (int i = 0; i < rows; i++)
		for(int j = 0; j < (2 *counters[i]); j += 2)
			sums_array[i] += w[i][j + 1];

	*sums = sums_array; 
}

double
update_y(double *y_new, double *sums_array, double **y_updated)
{
	double norm = 0, m_dif;
	double *y = *y_updated;

	for (int i = 0; i < rows; i++)
		for (int j = 0; j < columns; j++){
			y_new[i * columns + j] /= sums_array[i];
			m_dif = y_new[i * columns + j] - y[i * columns + j];
			norm += pow(m_dif, 2);

			y[i * columns + j] = y_new[i * columns + j];	
		}
	
	*y_updated = y;
	return norm;
}

double 
compute_distance(double *y_i, double *x_j, double limit)
{
	double dist = 0, tmp;
	for (int i = 0; i < columns; i++){
		tmp = y_i[i] - x_j[i];
		dist += tmp * tmp;
		if (dist > limit)
			return 0;
	}

	return exp( -dist / (2 * limit));
}

/* computes the exp for the elements inside the radius */
void 
get_exp_sparse(double *x, double *y, double h, double ***w_updated, int **counter_per_row)
{
	// array with distances for every element
	double **dists = *w_updated;
	int *distances_counter = *counter_per_row;

	// distances and indexes for every row
	double *row_dist = malloc(sizeof *row_dist * rows);
	double *row_idxs = malloc(sizeof *row_idxs * rows);

	// counter for every row
	int row_counter;

	double dist = 0;
	double limit = pow(h, 2);
	double sigma_square = h * h;
	
	for (int i = 0; i < rows; i++){
		row_counter = 0;
		
		for (int j = 0; j < rows; j++){

			dist = compute_distance(&y[i * columns], &x[j * columns], limit);

			// dist = (dist > limit) ? 0 : exp( -dist / (2 * sigma_square));
			
			if (dist){
				row_dist[row_counter] = dist;
				row_idxs[row_counter] = j;
				row_counter++;
			}
		}

		//update distances
		dists[i] = malloc(sizeof **dists * row_counter * 2);
		for (int k = 0; k < (2 * row_counter); k += 2){
			dists[i][k] = row_idxs[k / 2];
			dists[i][k + 1] = row_dist[k / 2];
		}
		distances_counter[i] = row_counter;
	}

	*w_updated = dists;
	*counter_per_row = distances_counter;

	// clean up
	free(row_dist);
	free(row_idxs);
}

double *
read_data(char *name)
{
	FILE *f;
	f = fopen(name, "rb");
	fseek(f, 0L, SEEK_END);
	int pos = ftell(f);
	fseek(f, 0L, SEEK_SET);

	int number_elements = pos / sizeof(double);
	double *x = malloc(sizeof *x * number_elements);
	fread(x, sizeof *x, number_elements, f);
	rows = number_elements / columns;
	fclose(f);

	return x;
}

int 
check_results(double *y, char *name)
{
	double *mat_res = read_data(name);
	double dist = 0;
	for (int i = 0; i < rows; i++){
		for (int j = 0; j < columns; j++){
			
			dist += pow(y[i * columns + j] - mat_res[i * columns + j], 2);
			// if (dist > 1)
			// 		printf("%f %f\n", y[i * columns + j], mat_res[i * columns + j]);
		}
	}
	printf("dist %f\n", dist);
	return 0;
}

void
write_results(char **argv, int iterations, double time)
{
  FILE *f;
  f = fopen("results_cpu.txt", "a");
  fprintf(f, "%d %d %s %d %f\n", rows, columns, argv[1], iterations, time);
  fclose(f);
}
