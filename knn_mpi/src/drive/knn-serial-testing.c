#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <limits.h>
#include <sys/time.h>
#include <string.h>
#include <omp.h>

// static int const N = 50;

struct timeval startwtime, endwtime;
double seq_time;

// initialize array 
void init_array(void);
void init_neighbors(void);
void init_neighbors_dist(void);

// load mnist files
void load_labels(char *name);
void load_examples(char *name);

// find k-nn for every spot
void find_kneighbors(void);

void print_neighbors(void);

int check_labes(void);

double *array, *train, *labels, *comp_array;
int **neighbors, files;
double **neighbors_dist;
int k,rows,columns;

int main(int argc, char **argv)
{
	if (argc < 3){
		fprintf(stderr, "%s\n", "Needs 2 arguments first files second k");
		exit(1);
	}
	k = atoi(argv[2]) + 1;
	char labels_path[100], examples_path[100];
	
	files = atoi(argv[1]);
	if (files > 2 || files < 1){
		fprintf(stderr, "Arguments can take as value 1 or 2\n");
		exit(1);
	}
	else if (files == 1){
		strcpy(labels_path, "data/train_768_labels.bin");
		strcpy(examples_path, "data/train_768.bin");
	}
	else{
		strcpy(labels_path, "data/train_30_labels.bin");
		strcpy(examples_path, "data/train_30.bin");
	}

	load_labels(labels_path);
	load_examples(examples_path);
	
	init_neighbors();
	init_neighbors_dist();
	
	gettimeofday (&startwtime, NULL);
	find_kneighbors();
	gettimeofday (&endwtime, NULL);
	seq_time = (double)((endwtime.tv_usec - startwtime.tv_usec)/1.0e6
		      + endwtime.tv_sec - startwtime.tv_sec);
	printf("%s wall clock time = %f\n","serial knn", seq_time);	
	check_labes();
	
	// free memory 
	for (int i=0; i < rows; i++){
		// free(array[i]);
		free(neighbors[i]);
	}
	// free(array);
	free(neighbors);
	free(neighbors_dist);
	return 0;
}

void load_labels(char *name)
{
	FILE *f;
	f = fopen(name, "rb");
	if (f == NULL) 
		perror("Error");

	// see how many elements
	fseek(f, 0L, SEEK_END);
	long int pos = ftell(f);
	fseek(f, 0L, SEEK_SET);
	int number_elements = pos / sizeof(double);
	// number_elements = 7;
	labels = malloc(sizeof *labels * number_elements);
	size_t nfils = fread(labels, sizeof(double), number_elements, f);
	if (!nfils){
		puts("Error reading the file");
		exit(1);
	}
	fclose(f);
	rows = number_elements;
}

void load_examples(char *name)
{
	FILE *f;
	f = fopen(name, "rb");
	if (f == NULL) 
		perror("Error");

	// see how many elements
	fseek(f, 0L, SEEK_END);
	long int pos = ftell(f);
	fseek(f, 0L, SEEK_SET);
	int number_elements = pos / sizeof(double);

	train = malloc(sizeof *train * number_elements);
	size_t nfils = fread(train, sizeof(double), number_elements, f);
	if (!nfils){
		puts("Error reading the file");
		exit(1);
	}
	fclose(f);
	columns = number_elements / rows;
	
	array = malloc(sizeof *array * rows * columns);
	comp_array = malloc(sizeof *array * rows * columns);
	for (int i = 0; i < rows; i++){
		for(int j = 0; j < columns; j++){
			array[i * columns + j] = train[i * columns + j];
		}
	}
	
	memcpy(comp_array, array, sizeof *array * rows * columns);
}

void init_neighbors(void)
{
	neighbors = malloc(rows * sizeof(int *));
	for (int i=0; i < rows; i++){
		neighbors[i] = malloc(k * sizeof(int));
		for (int j=0;j<k;j++)
			neighbors[i][j] = 0;		
	}

}

void init_neighbors_dist(void)
{
	neighbors_dist = malloc(rows * sizeof(double *));
	for (int i=0; i<rows; i++){
		neighbors_dist[i] = malloc(k * sizeof(double));
		for (int j=0; j<k; j++)
			neighbors_dist[i][j] = (double)INT_MAX;
	}
}

void exchange(int index, double dist, int i, int j)
{		
	double temp_dist;
	double temp_pos;
	int index_pos = index;

	// change dist
	for(index; index<k; index++){
		temp_dist = neighbors_dist[i][index];
		neighbors_dist[i][index] = dist;
		dist = temp_dist;
	}

	// change index
	for (index_pos; index_pos<k; index_pos++){
		temp_pos = neighbors[i][index_pos];
		neighbors[i][index_pos] = j;
		j = temp_pos;
	}
}

int givePar(int up)
{
	int sum = 0;
	for(int i = 0; i < up; i++)
		sum += rows - (i+1);
	// printf("sum %d\n", sum);
	return sum;
}

double norm(double *r)
{
	double res = 0;
	for (int i = 0; i < columns; i++)
		res += r[i] * r[i];
	return res;
}

void find_kneighbors(void)
{	
	
	double dist;
	double tmp;
	int N = 50;
	double dists[N][N];
	// norms of rows
	double norms_rows[N];
	double norms_cols[N];
	
	// #pragma omp parallel for private(norms_rows, norms_cols, dists)
	for (int i = 0; i < rows; i += N){
			
			for (int k1 = i; (k1 < (i + N) && k1 < rows); k1++)
				norms_rows[k1 - i] = norm(&array[k1 * columns]);
			
			for (int j = 0; j < rows; j += N){	
				
				for (int k1 = j; (k1 < (j + N) && k1 < rows); k1++)
					norms_cols[k1 - j] = norm(&comp_array[k1 * columns]);
				
				for (int k1 = i; (k1 < (i + N) && k1 < rows); k1++)
					for (int l = j; (l < (j + N) && l < rows); l++){
						tmp = 0;
						for (int m = 0; m < columns; m++)
							tmp -= array[k1 * columns + m] * comp_array[l * columns + m];
						
						dists[k1 - i][l - j] = norms_rows[k1 - i] + norms_cols[l - j] + 2 * tmp;	
					}		

			

			for (int k1 = 0; k1 < N; k1++){
				for (int l = 0; l < N; l++){

					if (dists[k1][l] <= neighbors_dist[k1 + i][k-1]){
						for(int index = 0; index < k; index++){		
							// insert in the que
							if (dists[k1][l] <= neighbors_dist[k1 + i][index]){
								// exchange for current index
								exchange(index, dists[k1][l], k1 + i, l + j);
								break;
							}
						} 
					}
				}
			}
		}
		
	}
	
}

double *
load_correct(char *name)
{
	
	FILE *f = fopen(name, "r");
	fseek(f, 0L, SEEK_END);
	long int pos = ftell(f);
	fseek(f, 0L, SEEK_SET);
	int num = pos / sizeof(double);
	double *ar = malloc(sizeof *ar * num);
	size_t n = fread(ar , sizeof(double), num, f);

	if (n){
		fclose(f);
		return ar;
	}
}

int check_labes(void)
{
	// init predictions
	double *predictions = malloc(sizeof *predictions * rows);

	char correct_path[100];
	if (files == 1)
		strcpy(correct_path, "data/correct768.bin");
	else
		strcpy(correct_path, "data/correct.bin");
	double *matlab_neighbors = load_correct(correct_path);

	// array with sum of every prediction
	int *sums = malloc(sizeof *sums * 10);
	int max, pred, ind;
	int c_wrong = 0;
	for (int i = 0; i < rows; i++){
		// initialize sums
		memset(sums, 0, sizeof(int) * 10);
		for (int j = 1; j < k; j++){
			ind = labels[neighbors[i][j]] - 1;
			if (neighbors[i][j] != (matlab_neighbors[i * 128 + (j - 1)] - 1)){
				/* at second file correct768 some labels of elements with differnt labels
				and some distances are reversed example matlab_indexes: 10 35, code_indexes:35 10*/
				c_wrong++;
				if (c_wrong == 200)
					return 1;
			}	
			sums[ind]++;	
		}
		
		// find max value
		max = sums[0];
		pred = 1;
		for (int j = 1; j < 10; j++){
			if (sums[j] > max){
				max = sums[j];
				pred = j + 1;
			}
		}
		// printf("%d \n", i);
		predictions[i] = pred;
	}

	int correct = 0;
	// check with labels
	for (int i = 0; i < rows; i++){
		if ( predictions[i] == labels[i] )
			correct++;
	}
	printf("correct %d\n", correct);
	float percentage = ((float) correct) / ((float) rows);
	printf("accuracy %.3f \n", percentage);

	return 0;
}
