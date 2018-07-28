#include "data_parser.h"
#include <stdlib.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

Sparse* create_adjacency(int N, Sparse_list *dataset);

int 
main(int argc, char **argv)
{
	int N = 0;
	char *dataset_path = argv[1];

	/* Read the dataset and count the nodes.*/
	Sparse_list *array = parse_data(dataset_path);	
	Sparse_list *dataset = array;
	while (array != NULL){
		array = array->next;
		N++;
	}
	printf("%d\n", N);
	Sparse *adjacency = create_adjacency(N, dataset);

	return 0;
}

Sparse*
create_adjacency(int N, Sparse_list *dataset)
{
	/* Find the total elements of every row of sparse matrix*/
	int *row_sums;
	if ((row_sums = malloc(N * sizeof *row_sums)) == NULL)
	{
		perror("Malloc at creation of row_sums");
		exit(1);
	}
	memset(row_sums, 0, N);
	
	Sparse_list element = *dataset;
	for (int i = 0; i < N; i++)
	{
		row_sums[element.cell.row - 1]++;
		if (element.next != NULL)
			element = *(element.next);
	}

	Sparse *adjacency;
	if ((adjacency = malloc(N * sizeof *adjacency)) == NULL)
	{
		perror("Malloc at creation of adjacency");
		exit(1);
	}
	element = *dataset;
	for (int i = 0; i < N; i++)
	{
		adjacency[i] = element.cell;
		adjacency[i].value /= row_sums[element.cell.row - 1];
		if (element.next != NULL)
			element = *(element.next);
		printf("%d %d %f \n", adjacency[i].row, adjacency[i].col, adjacency[i].value);
	}
	
	return adjacency;
}
