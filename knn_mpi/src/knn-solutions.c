#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <mpi.h>
#include <string.h>
#include <omp.h>
#include "knn-solutions.h"
#include "helper.h"
#include "globals.h"


/* Find the k-nearest neighbors of a dataset using the serial algorithm.
 *
 * Arguments:
 * array - The complete dataset.
 *
 * Output:
 * An array with the k nearest neighbors for every element.
 */
int*
find_kneighbors_serial(double *array)
{	
	/* Initialize the neighbors and their initial distances.*/
	int *neighbors = init_neighbors();
	double **neighbors_dist = init_neighbors_dist();

	int N = 50;  // How many elements has every batch of files.
	
	/* Allocate the distances of every batch*/
	double **dists = malloc(N * sizeof *dists);
	for (int i = 0; i < N; i++)
		dists[i] = malloc(N * sizeof **dists);

	/* Allocate the norms arrays.*/
	double *norms_rows = malloc(N * sizeof *norms_rows);
	if (norms_rows == NULL)
	{
		perror("Malloc norms_rows");
		exit(1);
	}
	double *norms_cols = malloc(N * sizeof *norms_cols);
	if (norms_cols == NULL)
	{
		perror("Malloc norms_cols");
		exit(1);
	}
	
	/* A copy of the initial array for comparison*/
	double *comp_array = malloc(sizeof *comp_array * rows * columns);
	if (comp_array == NULL)
	{
		perror("Malloc comp_array");
		exit(1);
	}
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

			update_distances(neighbors, neighbors_dist,
					 dists, array, 
					 comp_array, rows,
					 norms_rows, norms_cols, 
					 i, j, N, 0);
		}
	}
	
	return neighbors;
}

/* Translate the local index to global in the MPI environment
 *
 * Arguments:
 * mes -- The iteration's number.
 * j -- The local index.
 *
 * Output:
 * The global index.
 */
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

/* A function for computing the distances of all elements for a batch. For the 
 * computation of the distance x^2 + y^2 - 2*x*y is being used.
 *
 * Arguments:
 * dists -- Array with the distances to be filled.
 * array -- The row elements.
 * comp_array -- The column elements.
 * size_comp -- The number of rows for the comp_array.
 * norms_rows -- Norms of the array elements.
 * norms_cols -- Norms of the comp_array elements.
 * i -- Start index of the array rows.
 * j -- Start index of the comp_array rows.
 * N -- Batch size.
 */
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

/* Function for updating the distances of the nearest neighbors.
 *
 * Arguments:
 * dists -- Array with the distances to be filled.
 * array -- The row elements.
 * comp_array -- The column elements.
 * size_comp -- The number of rows for the comp_array.
 * norms_rows -- Norms of the array elements.
 * norms_cols -- Norms of the comp_array elements.
 * i -- Start index of the array rows.
 * j -- Start index of the comp_array rows.
 * N -- Batch size.
 */
void
update_distances(int *neighbors, double **neighbors_dist,
		 double **dists, double *array, 
	         double *comp_array, int size_comp,
	         double *norms_rows, double *norms_cols,
	         int i, int j,
	         int N, int mes)
{
	/* Find the distances.*/
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

						exchange(neighbors,
							 neighbors_dist,
							 index, 
							 dists[row - i][col - j],
							 row,
							 (col - j) + global_j);

						break;
					}
		}
}

/* Find the k-nearest neighbors of a dataset using the mpi blocking algorithm.
 *
 * Arguments:
 * array - The complete dataset.
 *
 * Output:
 * An array with the k nearest neighbors for batch's elements.
 */
int* 
find_kneighbors_blocking(double *array)
{	

	/* Initialize the neighbors and their initial distances.*/
	int *neighbors = init_neighbors();
	double **neighbors_dist = init_neighbors_dist();

	int N = 50;  // How many elements has every batch of files.
	
	/* Allocate the distances of every batch*/
	double **dists = malloc(N * sizeof *dists);
	for (int i = 0; i < N; i++)
		dists[i] = malloc(N * sizeof **dists);

	/* Allocate the norms arrays.*/
	double *norms_rows = malloc(N * sizeof *norms_rows);
	if (norms_rows == NULL)
	{
		perror("Malloc norms_rows");
		exit(1);
	}
	double *norms_cols = malloc(N * sizeof *norms_cols);
	if (norms_cols == NULL)
	{
		perror("Malloc norms_cols");
		exit(1);
	}
	
	/* A copy of the initial array for comparison*/
	double *comp_array = malloc(sizeof *comp_array * rows * columns);
	if (comp_array == NULL)
	{
		perror("Malloc comp_array");
		exit(1);
	}
	for (int i = 0; i < rows * columns; i++)
		comp_array[i] = array[i];
	int size_comp = rows;  // The number of rows of the comparison array.


	/* Initialize of MPI environment.*/
	MPI_Status status;
	
	/* Initialize receiver and transmitter.*/
	int send_to = (rank + 1 == world_size) ? 0 : (rank + 1);
	int receive_from = (rank == 0) ? (world_size - 1) : (rank - 1);

	/* Variables that will be used for the communication between the 
	 * processes.*/
	double *receiver, tmp;
	int rec_size;
	
	/* Ring communication.*/
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

				update_distances(neighbors, neighbors_dist,
						 dists, array, 
						 comp_array, size_comp,
						 norms_rows, norms_cols, 
						 i, j, N, 0);
			}
		}

		/* Terminate the ring communication.*/
		if (mes == world_size - 1)
			break;
		
		/* The processes with even rank first sending the data and 
		 * the processes with odd rank first receiving the data.*/
		if (!(rank % 2)){	
			MPI_Send(comp_array, size_comp * columns,
				 MPI_DOUBLE, send_to,
				 0, MPI_COMM_WORLD);
			
			/* Receive*/
			MPI_Probe(receive_from, 0, MPI_COMM_WORLD, &status);
			MPI_Get_count(&status, MPI_DOUBLE, &rec_size);	
						
			/* Re initialize the receive bufffer.*/
			receiver = malloc(sizeof *receiver * rec_size);	
			MPI_Recv(receiver, rec_size,
				 MPI_DOUBLE, receive_from,
				 0, MPI_COMM_WORLD,
				 MPI_STATUS_IGNORE);
			
			/* Re initialize the comp buffer.*/
			free(comp_array);
			comp_array = malloc(sizeof *comp_array * rec_size);
			for(int k = 0; k < rec_size; k++)
				comp_array[k] = receiver[k];
			free(receiver);
			size_comp = rec_size;
			size_comp /= columns;
		}
		else{
			/* Receive*/
			MPI_Probe(receive_from, 0, MPI_COMM_WORLD, &status);
			MPI_Get_count(&status, MPI_DOUBLE, &rec_size);

			/* Re initialize the receive bufffer.*/
			receiver = malloc(sizeof *receiver * rec_size);
			MPI_Recv(receiver, rec_size,
				 MPI_DOUBLE, receive_from,
				 0, MPI_COMM_WORLD,
				 MPI_STATUS_IGNORE);
			
			/* Send*/
			MPI_Send(comp_array, size_comp * columns,
				 MPI_DOUBLE, send_to,
				 0, MPI_COMM_WORLD);
			
			/* Re initialize the comp buffer.*/
			free(comp_array);
			comp_array = malloc(sizeof *comp_array * rec_size);
			for(int k = 0; k < rec_size; k++)	
				comp_array[k] = receiver[k];
			free(receiver);
			size_comp = rec_size;
			size_comp /= columns;
		}
	}	

	return neighbors;
}

/* Find the k-nearest neighbors of a dataset using the mpi non-blocking 
 * algorithm.
 *
 * Arguments:
 * array - The complete dataset.
 *
 * Output:
 * An array with the k nearest neighbors for batch's elements.
 */
int* 
find_kneighbors_nonblocking(double *array)
{	
	/* Initialize the neighbors and their initial distances.*/
	int *neighbors = init_neighbors();
	double **neighbors_dist = init_neighbors_dist();

	int N = 50;  // How many elements has every batch of files.
	
	/* Allocate the distances of every batch*/
	double **dists = malloc(N * sizeof *dists);
	for (int i = 0; i < N; i++)
		dists[i] = malloc(N * sizeof **dists);

	/* Allocate the norms arrays.*/
	double *norms_rows = malloc(N * sizeof *norms_rows);
	if (norms_rows == NULL)
	{
		perror("Malloc norms_rows");
		exit(1);
	}
	double *norms_cols = malloc(N * sizeof *norms_cols);
	if (norms_cols == NULL)
	{
		perror("Malloc norms_cols");
		exit(1);
	}
	
	/* A copy of the initial array for comparison*/
	double *comp_array = malloc(sizeof *comp_array * rows * columns);
	if (comp_array == NULL)
	{
		perror("Malloc comp_array");
		exit(1);
	}
	for (int i = 0; i < rows * columns; i++)
		comp_array[i] = array[i];
	int size_comp = rows;  // The number of rows of the comparison array.


	/* Initialize of MPI environment.*/
	MPI_Status status;
	MPI_Request mpireq_s, mpireq_r;
	
	/* Initialize receiver and transmitter.*/
	int send_to = (rank + 1 == world_size) ? 0 : (rank + 1);
	int receive_from = (rank == 0) ? (world_size - 1) : (rank - 1);

	/* Variables that will be used for the communication between the 
	 * processes.*/
	double *receiver, tmp;
	int rec_size;
	int flag;
	
	/* Ring communication*/
	for (int mes = 0; mes < world_size; mes++){
		
		/* Start sending the buffer.*/
		MPI_Isend(comp_array, size_comp * columns, MPI_DOUBLE, 
			        send_to, 0, MPI_COMM_WORLD, &mpireq_s);
			
		/* Wait for answer on how many elements will receive.*/
		MPI_Iprobe(receive_from, 0, MPI_COMM_WORLD, &flag, &status);
		while (!flag)
			MPI_Iprobe(receive_from, 0, MPI_COMM_WORLD, &flag, &status);
		MPI_Get_count(&status, MPI_DOUBLE, &rec_size);
			
		/* Initialize receive buffer.*/
		receiver = malloc(sizeof *receiver * rec_size);
			
		/* Receive*/
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
		
		/* Sync processes.*/
		MPI_Barrier(MPI_COMM_WORLD);

		free(comp_array);
		comp_array = malloc(sizeof *comp_array * rec_size);
		for(int k = 0; k < rec_size ; k++)
			comp_array[k] = receiver[k];
		free(receiver);
		size_comp = rec_size;
		size_comp /= columns;
	}	

	return neighbors;
}
