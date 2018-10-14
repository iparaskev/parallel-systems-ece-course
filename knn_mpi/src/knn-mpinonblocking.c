#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <mpi.h>
#include <string.h>
#include <omp.h>

/* global variables */
int rows, columns, k, el_per_proc, files;
int rank, world_size;
int *neighbors,*all_neighbors;
double *array, **neighbors_dist, *labels;

/* helper functions */
void get_rows(char *name);
void get_columns(char *name);
void load_examples(char *name);
void load_labels(char *name);
void init_neighbors(void);
void init_neighbors_dist(void);
void find_kneighbors(void);
int check_labels(void);

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
	// initialize mpi environment 
	MPI_Init(NULL, NULL);

	// get number of processes
	MPI_Comm_size(MPI_COMM_WORLD, &world_size);

	// get process rank 
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);

	// rank 0 get columns
	if (!rank){
		load_labels(labels_path);
		get_columns(examples_path);
	}
	MPI_Bcast(&columns, 1, MPI_INT, 0, MPI_COMM_WORLD);

	get_rows(labels_path);
	load_examples(examples_path);
	
	init_neighbors();
	init_neighbors_dist();
	MPI_Barrier(MPI_COMM_WORLD);
	double start = MPI_Wtime(), total_time;
	find_kneighbors();
	MPI_Barrier(MPI_COMM_WORLD);
	total_time = MPI_Wtime() - start;
	if (!rank)
		printf("total time nonblocking %f\n", total_time);	
	
	int *starting_rec = malloc(sizeof *starting_rec * world_size);
	for (int i = 0; i < world_size; i++)
		starting_rec[i] = i * el_per_proc * k;
	
	int *rec_counts;
	if (!rank){
		load_labels(labels_path);
		
		int max_el = (rows - (world_size - 1) * el_per_proc);
		rec_counts = malloc(sizeof *rec_counts * world_size);
		for(int i = 0; i < (world_size - 1); i++)
			rec_counts[i] = el_per_proc * k;	
		rec_counts[world_size -1] = max_el * k;
		
		all_neighbors = malloc(sizeof *all_neighbors * rows * k * world_size);
	}
	int num = (rank == world_size - 1) ? rows : el_per_proc;
	
	MPI_Gatherv(neighbors, num * k, MPI_INT, all_neighbors, 
		          rec_counts, starting_rec, MPI_INT, 0, MPI_COMM_WORLD);
	if (!rank){
		if (check_labels())
			printf("Wrong result\n");
		
		free(all_neighbors);
		free(labels);
	}
	
	// clean up
	free(array);
	free(neighbors);

	MPI_Finalize();
	return 0;
}

void
get_rows(char *name)
{
	FILE *f;
	f = fopen(name, "rb");
	if (f == NULL) 
		perror("Error");

	// see how many elements
	fseek(f, 0L, SEEK_END);
	long int pos = ftell(f);
	if (rank == (world_size - 1))
		rows = (pos / sizeof (double)) - (world_size - 1) * 
	         ((pos / sizeof (double)) / world_size);
	else
		rows = (pos / sizeof (double)) / world_size;
	el_per_proc = (pos / sizeof (double)) / world_size;
}

void 
get_columns(char *name)
{
	FILE *f;
	f = fopen(name, "rb");
	if (f == NULL) 
		perror("Error");

	// see how many elements
	fseek(f, 0L, SEEK_END);
	long int pos = ftell(f);
	
	int number_elements;
	number_elements = pos / sizeof (double);
	columns = number_elements / rows;
	fclose(f);
}

void 
load_examples(char *name)
{
	FILE *f;
	f = fopen(name, "rb");
	if (f == NULL) 
		perror("Error");

	// see how many elements
	fseek(f, 0L, SEEK_END);
	long int pos = ftell(f);
	
	int number_elements;
	int all_el = pos / sizeof (double);
	number_elements = rows * columns;
	if (rank != (world_size - 1))
		fseek(f, rank * number_elements * sizeof(double), SEEK_SET);
	else
		fseek(f, (all_el - number_elements) * sizeof(double), SEEK_SET);

	array = malloc(sizeof *array * number_elements);
	size_t nfils = fread(array, sizeof(double), number_elements, f);
	if (!nfils){
		puts("Error reading the file");
		exit(1);
	}
	fclose(f);
	
}

void 
init_neighbors(void)
{
	neighbors = malloc(sizeof *neighbors * rows * k);
	memset(neighbors, 0, sizeof *neighbors * rows * k);
}

void 
init_neighbors_dist(void)
{
	neighbors_dist = malloc(rows * sizeof(double *));
	for (int i = 0; i < rows; i++){
		neighbors_dist[i] = malloc(k * sizeof(double));
		for (int j = 0; j < k; j++)
			neighbors_dist[i][j] = (double)INT_MAX;
	}
}

void 
exchange(int index, double dist, int i, int j)
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
		temp_pos = neighbors[i * k + index_pos];
		neighbors[i * k + index_pos] = j;
		j = temp_pos;
	}
}

double 
norm(double *r)
{
	double res = 0;
	for (int i = 0; i < columns; i++)
		res += r[i] * r[i];
	return res;
}

void 
find_kneighbors(void)
{	
	// array with elements for comparison
	// and their size
	double *comp_array = malloc(sizeof *comp_array * rows * columns);
	for (int i = 0; i < rows * columns; i++){
		comp_array[i] = array[i];
	}
	
	double *receiver, tmp;
	int size_comp = rows;
	int rec_size = size_comp, flag;
	
	MPI_Status status;
	MPI_Request mpireq_s, mpireq_r;

	// indexes for arrays
	int index_i, index_j;
	
	// send and receive next packet
	int send_to = (rank + 1 == world_size) ? 0 : (rank + 1);
	int receive_from = (rank == 0) ? (world_size - 1) : (rank - 1);
	
	int N = 50;
	double dists[N][N];
	// norms of rows
	double norms_rows[N];
	double norms_cols[N];
	// for every proc
	for (int mes = 0; mes < world_size; mes++){
		
		MPI_Isend(comp_array, size_comp * columns, MPI_DOUBLE, 
			        send_to, 0, MPI_COMM_WORLD, &mpireq_s);
			
		MPI_Iprobe(receive_from, 0, MPI_COMM_WORLD, &flag, &status);
			
		while (!flag)
			MPI_Iprobe(receive_from, 0, MPI_COMM_WORLD, &flag, &status);
		MPI_Get_count(&status, MPI_DOUBLE, &rec_size);
			
		// re initialize
		receiver = malloc(sizeof *receiver * rec_size);
			
		MPI_Irecv(receiver, rec_size, MPI_DOUBLE, receive_from, 
				        0, MPI_COMM_WORLD, &mpireq_r);

		// #pragma omp parallel for private(norms_rows, norms_cols, dists)
		for (int i = 0; i < rows; i += N){
			
			for (int k1 = i; (k1 < (i + N) && k1 < rows); k1++)
				norms_rows[k1 - i] = norm(&array[k1 * columns]);
			
			for (int j = 0; j < size_comp; j += N){	
				
				for (int k1 = j; (k1 < (j + N) && k1 < size_comp); k1++)
					norms_cols[k1 - j] = norm(&comp_array[k1 * columns]);
				
				for (int k1 = i; (k1 < (i + N) && k1 < rows); k1++)
					for (int l = j; (l < (j + N) && l < size_comp); l++){
						tmp = 0;
						for (int m = 0; m < columns; m++)
							tmp -= array[k1 * columns + m] * comp_array[l * columns + m];
						
						dists[k1 - i][l - j] = norms_rows[k1 - i] + norms_cols[l - j] + 2 * tmp;	
					}		

				for (int k1 = i; (k1 < (i + N) && k1 < rows); k1++)
					for (int l = j; (l < (j + N) && l < size_comp); l++){
						if (dists[k1 - i][l - j] <= neighbors_dist[k1][k-1]){
							for(int index = 0; index < k; index++)		
								// insert in the que
								if (dists[k1 - i][l - j] <= neighbors_dist[k1][index]){
									// exchange for current index

									// want global j
									int global_j;
									if (mes > rank)
										global_j = j + (world_size - 1) * el_per_proc - 
									                 ((mes - (rank + 1)) * el_per_proc); 
									else
										global_j = j + rank * el_per_proc - (mes * el_per_proc);
									
									exchange(index, dists[k1 - i][l - j], k1, (l - j) + global_j);
									break;
								}
						}
					}	
			}	
		}
		MPI_Wait(&mpireq_s, &status);
		MPI_Wait(&mpireq_r, &status);
		// sync processes
		MPI_Barrier(MPI_COMM_WORLD);

		free(comp_array);
		comp_array = malloc(sizeof *comp_array * rec_size);
		for(int k = 0; k < rec_size ; k++)
			comp_array[k] = receiver[k];
		free(receiver);
		size_comp = rec_size;
		size_comp /= columns;
	}	
}

void 
load_labels(char *name)
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
	labels = malloc(sizeof *labels * number_elements);
	size_t nfils = fread(labels, sizeof(double), number_elements, f);
	if (!nfils){
		puts("Error reading the file");
		exit(1);
	}
	fclose(f);
	rows = number_elements;
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

int  
check_labels(void)
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
			ind = labels[all_neighbors[i * k + j]] - 1;
			if (all_neighbors[i * k + j] != (matlab_neighbors[i * 128 + (j - 1)] - 1)){
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