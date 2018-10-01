#include <stdlib.h>
#include <stddef.h>
#include <stdio.h>
#include <sys/time.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include "data_parser.h"
#include "page_rank_serial.h"
#include "helper.h"
#include "pagerank_parallel.h"
#include "graph_coloring.h"
#include "globals.h"

int 
main(int argc, char **argv)
{
	/* Parse arguments.*/
	num_thread = 2;
	int help = 0;     
	int save = 0;      
	char *save_file = NULL;    
	int val = 0;    
	char *validate_file = NULL; 
	int parallel = 0;    
	int colored = 0;
	char *dataset_file = NULL;
	int opt;
	char *optstring = "hs:v:pcd:t:";

	while ((opt = getopt(argc, argv, optstring)) != -1)
	{
		switch (opt)
		{
			case 'h':
				help = 1;
				break;
			case 's':
				save = 1;
				save_file = optarg;
				break;
			case 'v':
				val = 1;
				validate_file = optarg;
				break;
			case 'p':
				parallel = 1;
				break;
			case 'c':
				colored = 1;
				break;
			case 'd':
				dataset_file = optarg;
				break;
			case 't':
				num_thread = atoi(optarg);
				break;
			case '?':
				if (optopt == 's' || optopt == 'v')
					fprintf (stderr, 
						 "Option -%c requires an argument.\n", 
						 optopt);
				else if (isprint(optopt))
					fprintf (stderr, 
						 "Unknown -%c option.\n", 
						 optopt);
				else
					fprintf (stderr,
						 "Unknown option character `\\x%x'.\n",
						 optopt);
				exit(1);
		}
	}

	if (help)
	{
		printf("-h: help\n"
			"-d: the input dataset\n"
			"-p: use parallel algorithm\n"
			"-t: the number of threads\n"
			"-c: use graph coloring\n"
			"-s: save results to given file after option\n"
			"-v: validate results using the vector from file after option\n");
		exit(1);
	}

	int N; 
	double *R;
	char *dataset_path = argv[1];
	double t_start, t_end;

	/* Read the dataset and count the nodes.*/
	printf("Reading the dataset..\n");
	Sparse_list *array = parse_data(dataset_file);	
	printf("Done.\n");
	N = count_elements(array);
	Sparse_half **adjacency = create_adjacency(N, array);

	/* Decide which method will be used*/
	if (colored)
	{
		srand(1000);
		/* graph preprocessing.*/
		list *graph = make_undirected(array); 
		int *color = coloring(graph);
		free(graph);

		/* Partition the graph*/
		list *borders = malloc(sizeof *borders);
		Sparse_half **A = partitions(adjacency, color, borders);
		free(color);

		if (parallel)
		{
			t_start = now();
			R = pagerank_par(A, borders);
			t_end = now();
		}
		else
		{
			t_start = now();
			R = pagerank(A);
			t_end = now();
		}

		/* Convert back the graph for validation*/
		double *R1 = malloc(rows * sizeof *R1);
		for (int i = 0; i < rows; i++)
			R1[indexes[i]] = R[i];
		free(R);
		free(borders);
		R = R1;
	}
	else
	{
		if (parallel)
		{
			/* Make the last row as the only border*/
			list *borders = malloc(sizeof *borders);
			borders->head = &borders->element;
			borders->tail = NULL;
			borders->element.value = rows;
			borders->element.next = NULL;
			t_start = now();
			R = pagerank_par(adjacency, borders);
			t_end = now();
			free(borders);

		}
		else
		{
			t_start = now();
			R = pagerank(adjacency);
			t_end = now();
		}

	}
	free(array);
	
	printf("Time passed %0.10f \n", elapsed_time(t_start, t_end));
	//printf("%0.10f\n", elapsed_time(t_start, t_end));

	if (save)
		save_results(R, rows, save_file);
	if (val)
		validate(R, rows, validate_file);

	/* Clean up */
	for (int i = 0; i < rows; i++)
		free(adjacency[i]);
	free(adjacency);

	return 0;
}

