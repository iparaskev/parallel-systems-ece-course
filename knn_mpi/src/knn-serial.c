#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <limits.h>
#include <sys/time.h>
#include <string.h>
#include <omp.h>
#include "helper.h"
#include "globals.h"

// static int const N = 50;

struct timeval startwtime, endwtime;
double seq_time;

double *comp_array;
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
	check_labels();
	
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

