#include <stdlib.h>
#include <stdio.h>
#include <helper.h>
#include <sparse.h>
#include <math.h>
#include <string.h>
#include <constants.h>
#include <sys/time.h>

/* Computes the complete adjacency matrix from the sparse list, also it fills
 * the rows with all zeros with a normal distribuded row.
 *
 * Arguments:
 * N -- the number of the cells of the initial sparse matrix
 * rows -- the number of rows of the sparse matrix
 * dataset -- a pointer to the start of the linked list that contains the 
 *            initial sparse
 * row_elements -- an array to store how many elements each row of the adjacency
 *		   matrix will have.
 *
 * Output:
 * The final sparse array.
 */ 
Sparse_half**
create_adjacency(int N, int rows, Sparse_list *dataset, int **row_elements)
{
	/* Find the total outgoing link for every node.*/
	/* An array to store the number of outgoing edges of every node*/
	int *outgoing;  
	if ((outgoing = malloc(rows * sizeof *outgoing)) == NULL)
	{
		perror("Malloc at creation of outgoing");
		exit(1);
	}
	memset(outgoing, 0, rows); 

	/* An array to store the number of incoming edges of every node*/
	int *incoming; 
	if ((incoming = malloc(rows * sizeof *incoming)) == NULL)
	{
		perror("Malloc at creation of incoming");
		exit(1);
	}
	memset(incoming, 0, rows); // Initialization of the sums array.

	/* Go through the list to count the incoming and outgoing edges*/
	Sparse_list *current = dataset;
	while (current != NULL)
	{
		outgoing[current->cell.row - 1]++;
		incoming[current->cell.col - 1]++;
		current = current->next;
	}

	/* Create the adjacency matrix.*/
	Sparse_half **adjacency;
	if ((adjacency = malloc(rows * sizeof *adjacency)) == NULL)
	{
		perror("Malloc at creation of adjacency");
		exit(1);
	}
	/* Initialize the rows.*/
	for (int i = 0; i < rows; i++)
	{
		adjacency[i] = malloc(incoming[i] * sizeof **adjacency);
		if (adjacency[i] == NULL)
		{
			perror("Malloc at creation of adjacency");
			exit(1);
		}
	}

	/* An array for tracking at each row at which element we are.*/
	int *indexes;
	if ((indexes = malloc(rows * sizeof *indexes)) == NULL)
	{
		perror("Malloc");
		exit(1);
	}
	memset(indexes, 0, rows);

	/* Fill the adjacency matrix.*/
	current = dataset;
	int row, column;
	while (current != NULL)
	{
		row = current->cell.col - 1; 
		column = indexes[row];
		indexes[row]++;
		adjacency[row][column].col = current->cell.row - 1;
		adjacency[row][column].value = -D * 1./outgoing[current->cell.row - 1];
		current = current->next;
	}
	
	*row_elements = incoming;
	return adjacency;
}

/* Count the elements of the sparse dataset and find the number of unique nodes.
 *
 * Arguments:
 * rows -- the variable where the last index will be saved
 * array -- the linked list with the sparse array
 *
 * Output:
 * N -- the number of sparse cells
 */
int 
count_elements(int *rows, Sparse_list *array)
{
	int last_index = -1;
	int N = 0;
	while (array != NULL){
		if (array->cell.row > last_index)
			last_index = array->cell.row;
		if (array->cell.col > last_index)
			last_index = array->cell.col;
		array = array->next;
		N++;
	}

	*rows = last_index;
	return N;
}

/* Compute the l1 norm of two vectors of size N.*/
double
norm(double *a, double *b, int N)
{
	double sum = 0;
	for (int i = 0; i < N; i++)
		sum += fabs(a[i] - b[i]);

	return sum;
}

void
normalize(double **x_init, int rows)
{
	double sumo = 0;
	double *x = *x_init;
	for (int i = 0; i < rows; i++)
		sumo += fabs(x[i]);
	for (int i = 0; i < rows; i++)
		x[i] /= sumo;
}

/* Save pagerank result at a binary file.*/
void 
save_results(double *x, int N)
{
	FILE *f = fopen("results.bin", "wb");
	if (f == NULL)
	{
		perror("Opening the results file");
		exit(1);
	}
	if (((int) fwrite(x, sizeof *x, N, f)) != N)
	{
		fprintf(stderr, "Error writting the results");
		exit(1);
	}	
	fclose(f);
}

/* Validate the results using the 'groundtruth' from matlab code*/
void
validate(double *x, int N, char *filename)
{
	/* Read data from file.*/
	FILE *f = fopen(filename, "rb");
	if (f == NULL)
	{
		perror("Opening the file.");
		exit(1);
	}
	double *matlab_pagerank;
	if ((matlab_pagerank = malloc(N * sizeof *matlab_pagerank)) == NULL)
	{
		perror("Malloc at reading validation vector.");
		exit(1);
	}
	if (((int) fread(matlab_pagerank, sizeof *matlab_pagerank, N, f)) != N)
	{
		fprintf(stderr, "Error reading the results");
		exit(1);
	}	

	/* Get the norm of the two vectors.*/
	double difference = norm(x, matlab_pagerank, N);
	printf("The l1 norm of the two vectors is %.10f.\n", difference);
}

/* Get current timestamp at seconds*/
double
now()
{
	struct timeval tz;
	gettimeofday(&tz, NULL);
	return tz.tv_sec + tz.tv_usec / 1000000.0;
}

/* Compute the elapsed time between two timestaps
 *
 * Arguments:
 * t_start -- the first timestamp
 * t_end -- the second timestamp
 *
 * Output:
 * The elapsed time in second between the two timestamps.
 */
double
elapsed_time(double start, double end)
{
	return end - start;
}
