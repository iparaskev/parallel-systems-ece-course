#include "data_parser.h"
#include "page_rank_serial.h"
#include "helper.h"
#include "pagerank_parallel.h"
#include <stdlib.h>
#include <stddef.h>
#include <stdio.h>
#include <sys/time.h>
#include <string.h>

int 
main(int argc, char **argv)
{
	int N, rows, *row_sums; 
	double *R;
	char *dataset_path = argv[2];
	double t_start, t_end;

	/* Read the dataset and count the nodes.*/
	Sparse_list *array = parse_data(dataset_path);	
	N = count_elements(&rows, array);
	printf("%d %d\n", N, rows);
	Sparse_half **adjacency = create_adjacency(N, rows, array, &row_sums);
	free(array);
	puts("ok adj");
	/* Begin pagerank process*/
	t_start = now();
	if (!strcmp(argv[1], "par"))
		R = pagerank_par(adjacency, rows, row_sums);
	else
		R = pagerank(adjacency, rows, row_sums);
	t_end = now();
	printf("Time passed %0.10f \n", elapsed_time(t_start, t_end));
	save_results(R, rows);
	free(adjacency);

	return 0;
}

