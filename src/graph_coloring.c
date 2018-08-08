#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "data_parser.h"
#include "helper.h"

struct con_nodes
{
	int index;
	struct con_nodes *next;
};

struct con_nodes** make_undirected(int rows, Sparse_list *data, int **row_sums);
void und(Sparse_half **A, int rows, int *row_sums);

int 
main(int argc, char **argv)
{
	int N, rows, *row_sums, *connections;
	Sparse_list *data = parse_data(argv[1]);
	N = count_elements(&rows, data);
	printf("N %d rows %d\n", N, rows);
	//Sparse_half **adjacency = create_adjacency(N, rows, data, &row_sums);
	int **r = malloc(rows * sizeof *r);
	for (int i = 0; i < rows; i++)
	{
		r[i] = malloc(rows * sizeof **r);
	}
	//struct con_nodes **undirected = make_undirected(rows, data, &connections);
	//und(adjacency, rows, row_sums);

	return 0;
}


void
append(struct con_nodes *current, int index)
{
	struct con_nodes *next = malloc(sizeof *next);
	if (next == NULL)
	{
		perror("Malloc");
		exit(1);
	}
	next->index = index;
	next->next = NULL;
	current->next = next;
}

void
und(Sparse_half **A, int rows, int *row_sums)
{
	for (int row = 0; row < rows; row++)
	{
		for (int col = 0; col < row_sums[row]; col++)
		{
			int found = 0;
			for (int j = 0; j < row_sums[col]; j++)
			{
				if (A[col][j].col == row)
				{
					found = 1;
					break;
				}
			}

			if (!found)
			{
				row_sums[col]++;
				printf("%d %d\n", row_sums[col], col);
				if (row_sums[col] == 1)
					A[col] = malloc(sizeof **A);
				else
					A[col] = realloc(A[col], row_sums[col]);
				A[col][row_sums[col] - 1].col = row;
				A[col][row_sums[col] - 1].value = 1;
			}
		}
	}
}

struct con_nodes**
make_undirected(int rows, Sparse_list *data, int **row_sums)
{
	Sparse_list *current = data;

	/* An array with lists with the indexes of every connected node */
	struct con_nodes **undirected;
	undirected = malloc(rows * sizeof *undirected);
	if (undirected == NULL)
	{
		perror("Malloc");
		exit(1);
	}
	
	/* Find the size of every row on the undirected graph */
	int *connections;
	if ((connections = malloc(rows * sizeof *connections)) == NULL)
	{
		perror("Malloc");
		exit(1);
	}
	memset(connections, 0, rows);
	struct con_nodes *head;
	int counter = 0;
	while (current != NULL)
	{
		int row_index = current->cell.row - 1;
		int col_index = current->cell.col - 1;

		if (counter % 1000 == 0)
			printf("counter %d\n", counter);
		counter++;
		if (!connections[row_index])
		{
			undirected[row_index] = malloc(sizeof **undirected);
			undirected[row_index]->index = col_index;
			undirected[row_index]->next = NULL;
			connections[row_index]++;
		}
		else
		{
			head = undirected[row_index];
			int found = 0;
			/* Go to the last entry of the list*/
			while (head->next != NULL)
			{
				if (head->index == col_index)
				{
					found = 1;
					break;
				}
				head = head->next;
			}

			/* Check the last element*/
			if (head->index == col_index)
				found = 1;

			/* If the value of the col_index isn't in at the list
			 * add it, else go to the next node
			 */
			if (!found)
			{
				append(head, col_index);
				connections[row_index]++;
			}
		}

		current = current->next;
	}

	current = data;
	counter = 0;
	while (current != NULL)
	{
		if (counter % 1000 == 0)
			printf("counter %d\n", counter);
		counter++;
		int row_index = current->cell.row - 1;
		int col_index = current->cell.col - 1;
		if (!connections[col_index])
		{
			undirected[col_index] = malloc(sizeof **undirected);
			undirected[col_index]->index = row_index;
			undirected[col_index]->next = NULL;
			connections[col_index]++;
		}
		else
		{
				head = undirected[col_index];
				int found = 0;
				/* Go to the last entry of the list*/
				while (head->next != NULL)
				{
					if (head->index == row_index)
					{
						found = 1;
						break;
					}
				head = head->next;
				}

				/* Check the last element*/
				if (head->index == row_index)
					found = 1;

				/* If the value of the col_index isn't in at the list
				 * add it, else go to the next node
				 */
				if (!found)
				{
					append(head, row_index);
					connections[col_index]++;
				}
		}
		current = current->next;
	}

	*row_sums = connections;
	return undirected;
}
