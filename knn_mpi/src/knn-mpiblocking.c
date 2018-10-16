#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <mpi.h>
#include <string.h>
#include <omp.h>
#include "helper.h"
#include "globals.h"

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
		printf("total time blocking %f\n", total_time);	
	
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

void find_kneighbors(void)
{	
	// array with elements for comparison
	// and their size
	double *comp_array = malloc(sizeof *comp_array * rows * columns);
	for (int i = 0; i < rows * columns; i++){
		comp_array[i] = array[i];
	}
	double *receiver, tmp;
	int size_comp = rows;
	int rec_size;
	
	int N = 50;
	
	double dists[N][N];
	// norms of rows
	double norms_rows[N];
	double norms_cols[N];

	MPI_Status status;

	// indexes for arrays
	int index_i, index_j;
	
	// send and receive next packet
	int send_to = (rank + 1 == world_size) ? 0 : (rank + 1);
	int receive_from = (rank == 0) ? (world_size - 1) : (rank - 1);
	
	// for every proc
	for (int mes = 0; mes < world_size; mes++){
		
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
		
		if (mes == world_size - 1)
			break;
		
		if (!(rank % 2)){	
			MPI_Send(comp_array, size_comp * columns, MPI_DOUBLE, send_to, 0, MPI_COMM_WORLD);
			
			// receive
			MPI_Probe(receive_from, 0, MPI_COMM_WORLD, &status);
			MPI_Get_count(&status, MPI_DOUBLE, &rec_size);	
			// re initialize
			receiver = malloc(sizeof *receiver * rec_size);	
			MPI_Recv(receiver, rec_size, MPI_DOUBLE, receive_from, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
			
			free(comp_array);
			comp_array = malloc(sizeof *comp_array * rec_size);
			for(int k = 0; k < rec_size; k++)
				comp_array[k] = receiver[k];
			free(receiver);
			size_comp = rec_size;
			size_comp /= columns;

		}
		else{
			// receive
			MPI_Probe(receive_from, 0, MPI_COMM_WORLD, &status);
			MPI_Get_count(&status, MPI_DOUBLE, &rec_size);
			receiver = malloc(sizeof *receiver * rec_size);
			MPI_Recv(receiver, rec_size, MPI_DOUBLE, receive_from, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
			
			// send
			MPI_Send(comp_array, size_comp * columns, MPI_DOUBLE, send_to, 0, MPI_COMM_WORLD);
			
			free(comp_array);
			comp_array = malloc(sizeof *comp_array * rec_size);
			for(int k = 0; k < rec_size; k++)	
				comp_array[k] = receiver[k];
			free(receiver);
			size_comp = rec_size;
			size_comp /= columns;
		}
	}	
}

