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
	//double dists[N][N];
	double **dists = malloc(N * sizeof *dists);
	for (int i = 0; i < N; i++)
		dists[i] = malloc(N * sizeof **dists);
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
			
		/* Rows norms*/
		n_norm(norms_rows, array, i, N, rows);
			
		for (int j = 0; j < rows; j += N)
		{	
			/* Columns norms*/
			n_norm(norms_cols, comp_array, j, N, rows);
			update_distances(dists, array, 
					 comp_array, rows,
					 norms_rows, norms_cols, 
					 i, j, N, 0);

		}
		
	}
	
}

int
translate_index(int mes, int j)
{
	int global_j;
	if (mes > rank)
		global_j = j + (world_size - 1)*el_per_proc\
			   - ((mes - (rank + 1))*el_per_proc); 
	else
		global_j = j + rank*el_per_proc - mes*el_per_proc;

	return global_j;

}

/* Compute the norms of N rows inside an array*/
void 
n_norm(double *norms, double *array, int start, int N, int find_end)
{
	for (int row = start; (row < (start + N) && (row < find_end)); row++)	
		norms[row - start] = norm(&array[row * columns]);
}

void
distances(double **dists, double *array, 
	  double *comp_array, int size_comp,
	  double *norms_rows, double *norms_cols,
	  int i, int j,
	  int N)
{
	double temp;
	for (int row = i; (row < (i + N) && row < rows); row++)
		for (int col = j; (col < (j + N) && col < size_comp); col++)
		{
			temp = 0;
			for (int in_col = 0; in_col < columns; in_col++)
				temp -= array[row*columns + in_col]\
				        * comp_array[col*columns + in_col];

			dists[row - i][col - j] = norms_rows[row - i]\
						  + norms_cols[col - j]\
						  + 2*temp;
		}
}

void
update_distances(double **dists, double *array, 
	         double *comp_array, int size_comp,
	         double *norms_rows, double *norms_cols,
	         int i, int j,
	         int N, int mes)
{
	distances(dists, array, comp_array, size_comp, 
		  norms_rows, norms_cols, i, j, N);

	/* Update neighbors.*/
	for (int row = i; (row < (i + N) && row < rows); row++)
		for (int col = j; (col < (j + N) && col < size_comp); col++)
		{
			if (dists[row - i][col - j] <= neighbors_dist[row][k - 1])
				for (int index = 0; index < k; index++)
					if (dists[row - i][col - j]
					    <= neighbors_dist[row][index])
					{
						// want global j
						int global_j = translate_index(mes, j);

						exchange(index, dists[row - i][col - j],
							 row, (col - j) + global_j);

						break;
					}
		}
}

void 
find_kneighbors_blocking (void)
{	
	// array with elements for comparison
	// and their size
	double *comp_array = malloc(sizeof *comp_array * rows * columns);
	for (int i = 0; i < rows * columns; i++)
		comp_array[i] = array[i];

	double *receiver, tmp;
	int size_comp = rows;
	int rec_size;
	
	int N = 50;
	double **dists = malloc(N * sizeof *dists);
	for (int i = 0; i < N; i++)
		dists[i] = malloc(N * sizeof **dists);

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
		for (int i = 0; i < rows; i += N)
		{
			/* Rows norms*/
			n_norm(norms_rows, array, i, N, rows);

			for (int j = 0; j < size_comp; j += N)
			{	
				/* Columns norms*/
				n_norm(norms_cols, comp_array, j, N, size_comp);
				
				update_distances(dists, array, 
						 comp_array, size_comp,
					         norms_rows, norms_cols, 
						 i, j, N, mes);
			}	
		}
		
		if (mes == world_size - 1)
			break;
		
		if (!(rank % 2)){	
			MPI_Send(comp_array, size_comp * columns,
				 MPI_DOUBLE, send_to,
				 0, MPI_COMM_WORLD);
			
			// receive
			MPI_Probe(receive_from, 0, MPI_COMM_WORLD, &status);
			MPI_Get_count(&status, MPI_DOUBLE, &rec_size);	
			// re initialize
			receiver = malloc(sizeof *receiver * rec_size);	
			MPI_Recv(receiver, rec_size,
				 MPI_DOUBLE, receive_from,
				 0, MPI_COMM_WORLD,
				 MPI_STATUS_IGNORE);
			
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
			MPI_Recv(receiver, rec_size,
				 MPI_DOUBLE, receive_from,
				 0, MPI_COMM_WORLD,
				 MPI_STATUS_IGNORE);
			
			// send
			MPI_Send(comp_array, size_comp * columns,
				 MPI_DOUBLE, send_to,
				 0, MPI_COMM_WORLD);
			
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
	//double dists[N][N];
	double **dists = malloc(N * sizeof *dists);
	for (int i = 0; i < N; i++)
		dists[i] = malloc(N * sizeof **dists);
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
		for (int i = 0; i < rows; i += N)
		{
			/* Rows norms*/
			n_norm(norms_rows, array, i, N, rows);

			for (int j = 0; j < size_comp; j += N)
			{	
				/* Columns norms*/
				n_norm(norms_cols, comp_array, j, N, size_comp);
				
				update_distances(dists, array, 
						 comp_array, size_comp,
					         norms_rows, norms_cols, 
						 i, j, N, mes);
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


