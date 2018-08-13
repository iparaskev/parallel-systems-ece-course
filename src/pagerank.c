#include "data_parser.h"
#include "page_rank_serial.h"
#include "helper.h"
#include "pagerank_parallel.h"
#include "graph_coloring.h"
#include "globals.h"
#include <stdlib.h>
#include <stddef.h>
#include <stdio.h>
#include <sys/time.h>
#include <string.h>

int 
main(int argc, char **argv)
{
	int N; 
	double *R;
	char *dataset_path = argv[1];
	double t_start, t_end;

	/* Read the dataset and count the nodes.*/
	Sparse_list *array = parse_data(dataset_path);	
	N = count_elements(array);
	//printf("%d %d\n", N, rows);
	Sparse_half **adjacency = create_adjacency(N, array);

	/* Decide which method will be used*/
	int par = atoi(argv[2]);
	if (par)
	{
		/* graph preprocessing.*/
		list *graph = make_undirected(array); 
		int *color = coloring(graph);
		free(graph);
		Sparse_half **A = partitions(adjacency, color);
		//for(int i = 0; i < 100; i++)
		//	printf("%d index %d\n", i, indexes[i]);
	
		t_start = now();
		R = pagerank(A);
		t_end = now();

		/* Convert back the graph for validation*/
		double *R1 = malloc(rows * sizeof *R1);
		for (int i = 0; i < rows; i++)
			R1[indexes[i]] = R[i];
		free(R);
		R = R1;
	}
	else
	{
		t_start = now();
		R = pagerank(adjacency);
		t_end = now();

	}
	free(array);
	
	/* Begin pagerank process*/
	printf("Time passed %0.10f \n", elapsed_time(t_start, t_end));

	for (int i = 0; i < 10; i++)
		printf("%f \n", R[i]);
	save_results(R, rows);
	free(adjacency);

	return 0;
}

