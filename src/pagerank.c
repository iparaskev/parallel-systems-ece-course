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
		srand(1000);
		/* graph preprocessing.*/
		list *graph = make_undirected(array); 
		int *color = coloring(graph);
		free(graph);
		free(array);

		/* Partition the graph*/
		list *borders = malloc(sizeof *borders);
		Sparse_half **A = partitions(adjacency, color, borders);
		linked_list *a = borders->head;
//		for (; a != NULL; a = a->next)
//			printf("%d\n", a->value);

		t_start = now();
		//R = pagerank(A);
		R = pagerank_par(A, borders);
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

	for (int i = 0; i < rows; i++)
		free(adjacency[i]);
	free(adjacency);

	return 0;
}

