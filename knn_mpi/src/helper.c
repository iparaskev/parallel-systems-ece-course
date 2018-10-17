#include <stdlib.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>
#include "helper.h"
#include "globals.h"

void 
load_labels (char *name)
{
	FILE *f;
	f = fopen(name, "rb");
	if (f == NULL) 
		perror("Error");

	// see how many elements
	fseek(f, 0L, SEEK_END);
	long int pos = ftell(f);
	fseek(f, 0L, SEEK_SET);
	int number_elements = pos / sizeof(double);

	labels = malloc(sizeof *labels * number_elements);
	if (labels == NULL)
	{
		perror("Malloc at reading the files");
		exit(1);
	}

	size_t nfils = fread(labels, sizeof(double), number_elements, f);
	if (!nfils)
	{
		puts("Error reading labels");
		exit(1);
	}
	fclose(f);
	rows = number_elements;
}

void 
load_examples(char *name)
{
	FILE *f;
	f = fopen(name, "rb");
	if (f == NULL) 
		perror("Error");

	// see how many elements
	fseek(f, 0L, SEEK_END);
	long int pos = ftell(f);
	
	int number_elements;
	int all_el = pos / sizeof (double);
	number_elements = rows * columns;
	if (rank != (world_size - 1))
		fseek(f, rank * number_elements * sizeof(double), SEEK_SET);
	else
		fseek(f, (all_el - number_elements) * sizeof(double), SEEK_SET);

	array = malloc(sizeof *array * number_elements);
	size_t nfils = fread(array, sizeof(double), number_elements, f);
	if (!nfils){
		puts("Error reading the file");
		exit(1);
	}
	fclose(f);
	
}

void
get_rows(char *name)
{
	FILE *f;
	f = fopen(name, "rb");
	if (f == NULL) 
		perror("Error");

	// see how many elements
	fseek(f, 0L, SEEK_END);
	long int pos = ftell(f);
	if (rank == (world_size - 1))
		rows = (pos / sizeof (double)) - (world_size - 1) * 
	         ((pos / sizeof (double)) / world_size);
	else
		rows = (pos / sizeof (double)) / world_size;
	el_per_proc = (pos / sizeof (double)) / world_size;
}

void 
get_columns(char *name)
{
	FILE *f;
	f = fopen(name, "rb");
	if (f == NULL) 
		perror("Error");

	// see how many elements
	fseek(f, 0L, SEEK_END);
	long int pos = ftell(f);
	
	int number_elements;
	number_elements = pos / sizeof (double);
	columns = number_elements / rows;
	fclose(f);
}

void 
init_neighbors(void)
{
	neighbors = malloc(sizeof *neighbors * rows * k);
	memset(neighbors, 0, sizeof *neighbors * rows * k);
}

void 
init_neighbors_dist(void)
{
	neighbors_dist = malloc(rows * sizeof(double *));
	for (int i = 0; i < rows; i++){
		neighbors_dist[i] = malloc(k * sizeof(double));
		for (int j = 0; j < k; j++)
			neighbors_dist[i][j] = (double)INT_MAX;
	}
}

void 
exchange(int index, double dist, int i, int j)
{		
	double temp_dist;
	double temp_pos;
	int index_pos = index;

	// change dist
	for(index; index<k; index++)
	{
		temp_dist = neighbors_dist[i][index];
		neighbors_dist[i][index] = dist;
		dist = temp_dist;
	}

	// change index
	for (index_pos; index_pos<k; index_pos++)
	{
		temp_pos = neighbors[i * k + index_pos];
		neighbors[i * k + index_pos] = j;
		j = temp_pos;
	}
}

double 
norm(double *r)
{
	double res = 0;
	for (int i = 0; i < columns; i++)
		res += r[i] * r[i];
	return res;
}

double *
load_correct(char *name)
{
	
	FILE *f = fopen(name, "r");
	fseek(f, 0L, SEEK_END);
	long int pos = ftell(f);
	fseek(f, 0L, SEEK_SET);
	int num = pos / sizeof(double);
	double *ar = malloc(sizeof *ar * num);
	size_t n = fread(ar , sizeof(double), num, f);

	if (n){
		fclose(f);
		return ar;
	}
}

int  
check_labels(void)
{
	// init predictions
	double *predictions = malloc(sizeof *predictions * rows);

	char correct_path[100];
	if (files == 1)
		strcpy(correct_path, "datasets/correct768.bin");
	else
		strcpy(correct_path, "datasets/correct.bin");
	double *matlab_neighbors = load_correct(correct_path);
	
	// array with sum of every prediction
	int *sums = malloc(sizeof *sums * 10);
	int max, pred, ind;
	
	int c_wrong = 0;
	for (int i = 0; i < rows; i++){
		// initialize sums
		memset(sums, 0, sizeof(int) * 10);
		for (int j = 1; j < k; j++){
			ind = labels[all_neighbors[i * k + j]] - 1;
			if (all_neighbors[i * k + j] != (matlab_neighbors[i * 128 + (j - 1)] - 1)){
				/* at second file correct768 some labels of elements with differnt labels
				and some distances are reversed example matlab_indexes: 10 35, code_indexes:35 10*/
				c_wrong++;
				if (c_wrong == 200)
					return 1;
			}	
			sums[ind]++;	
		}
		
		// find max value
		max = sums[0];
		pred = 1;
		for (int j = 1; j < 10; j++){
			
			if (sums[j] > max){
				max = sums[j];
				pred = j + 1;
			}
		}
		predictions[i] = pred;
	}

	int correct = 0;
	// check with labels
	for (int i = 0; i < rows; i++){
		if ( predictions[i] == labels[i] )
			correct++;
	}
	printf("correct %d\n", correct);
	float percentage = ((float) correct) / ((float) rows);
	printf("accuracy %.3f \n", percentage);

	return 0;
}


