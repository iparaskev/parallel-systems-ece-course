#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <mpi.h>
#include <string.h>
#include <omp.h>
#include "knn-solutions.h"
#include "helper.h"
#include "globals.h"

void 
find_kneighbors_serial (void)
{	
	
	double dist;
	double tmp;
	int N = 50;
	double dists[N][N];
	// norms of rows
	double norms_rows[N];
	double norms_cols[N];
	
	// array with elements for comparison
	// and their size
	double *comp_array = malloc(sizeof *comp_array * rows * columns);
	for (int i = 0; i < rows * columns; i++)
		comp_array[i] = array[i];

	// #pragma omp parallel for private(norms_rows, norms_cols, dists)
	for (int i = 0; i < rows; i += N)
	{
			
		for (int k1 = i; (k1 < (i + N) && k1 < rows); k1++)
			norms_rows[k1 - i] = norm(&array[k1 * columns]);
			
		for (int j = 0; j < rows; j += N)
		{	
			for (int k1 = j; (k1 < (j + N) && k1 < rows); k1++)
				norms_cols[k1 - j] = norm(&comp_array[k1 * columns]);
				
			for (int k1 = i; (k1 < (i + N) && k1 < rows); k1++)
				for (int l = j; (l < (j + N) && l < rows); l++)
				{
					tmp = 0;
					for (int m = 0; m < columns; m++)
						tmp -= array[k1 * columns + m] * comp_array[l * columns + m];
					
					dists[k1 - i][l - j] = norms_rows[k1 - i] + norms_cols[l - j] + 2 * tmp;	
				}		

			

			for (int k1 = 0; k1 < N; k1++)
				for (int l = 0; l < N; l++)
					if (dists[k1][l] <= neighbors_dist[k1 + i][k-1])
						for(int index = 0; index < k; index++)
							// insert in the que
							if (dists[k1][l] <= neighbors_dist[k1 + i][index])
							{
								// exchange for current index
								exchange(index, dists[k1][l], k1 + i, l + j);
								break;
							}
		}
		
	}
	
}


void 
find_kneighbors_blocking (void)
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

void 
find_kneighbors_nonblocking(void)
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


