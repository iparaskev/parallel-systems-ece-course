#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <limits.h>
#include <sys/time.h>
#include <string.h>
#include <omp.h>
#include <mpi.h>
#include "helper.h"
#include "globals.h"
#include "knn-solutions.h"

struct timeval startwtime, endwtime;
double seq_time;

double *comp_array;
int main(int argc, char **argv)
{
	if (argc < 4){
		fprintf(stderr, "%s\n", "Needs 2 arguments first files second k");
		exit(1);
	}

	/* Read arguments*/
	files = atoi(argv[1]);
	k = atoi(argv[2]) + 1;
	char *method = argv[3];

	/* Read datasets.*/
	char labels_path[100], examples_path[100];
	if (files > 2 || files < 1)
	{
		fprintf(stderr, "Arguments can take as value 1 or 2\n");
		exit(1);
	}
	else if (files == 1)
	{
		strcpy(labels_path, "dataset/train_768_labels.bin");
		strcpy(examples_path, "data/train_768.bin");
	}
	else
	{
		strcpy(labels_path, "dataset/train_30_labels.bin");
		strcpy(examples_path, "dataset/train_30.bin");
	}

	load_labels(labels_path);
	load_examples(examples_path);
	
	init_neighbors();
	init_neighbors_dist();
	
	if (strcmp(method, "serial") == 0)
	{
		gettimeofday (&startwtime, NULL);
		find_kneighbors_serial();
		gettimeofday (&endwtime, NULL);
		seq_time = (double)((endwtime.tv_usec - startwtime.tv_usec)/1.0e6
			      + endwtime.tv_sec - startwtime.tv_sec);
		printf("%s wall clock time = %f\n","serial knn", seq_time);	
		
		free(neighbors);
	}
	else
	{
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

		/* Call find knn*/
		if (strcmp(method, "blocking") == 0)
			find_kneighbors_blocking();
		else
			find_kneighbors_nonblocking();

		MPI_Barrier(MPI_COMM_WORLD);
		total_time = MPI_Wtime() - start;
		if (!rank)
			printf("total time blocking %f\n", total_time);	
		
		int *starting_rec = malloc(sizeof *starting_rec * world_size);
		for (int i = 0; i < world_size; i++)
			starting_rec[i] = i * el_per_proc * k;
		
		int *rec_counts;
		if (!rank)
		{
			load_labels(labels_path);
			
			int max_el = (rows - (world_size - 1) * el_per_proc);
			rec_counts = malloc(sizeof *rec_counts * world_size);
			for(int i = 0; i < (world_size - 1); i++)
				rec_counts[i] = el_per_proc * k;	
			rec_counts[world_size -1] = max_el * k;
			
			all_neighbors = malloc(sizeof *all_neighbors
					       * rows * k * world_size);
		}
		int num = (rank == world_size - 1) ? rows : el_per_proc;
		
		MPI_Gatherv(neighbors, num * k, 
			    MPI_INT, all_neighbors, 
			    rec_counts, starting_rec,
			    MPI_INT, 0,
			    MPI_COMM_WORLD);
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
	}

	return 0;
}


