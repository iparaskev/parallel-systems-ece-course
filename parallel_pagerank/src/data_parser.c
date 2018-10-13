#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "data_parser.h"
#include "helper.h"

#define MAX_LENGTH 50

Sparse_list* 
parse_data(char *filename)
{
	/*Initialize the datastructure.*/
	Sparse_list *data, *next, *current_node;
	if ((data = malloc(sizeof *data)) == NULL)
		err_exit("Malloc");

	current_node = data;

	/*Read file.*/
	FILE *fd;
	if ((fd = fopen(filename, "r")) == NULL)
		err_exit("Error opening the dataset file.");
	
	/* Initialize the parser's buffer*/
	char buffer[MAX_LENGTH];		
	memset(buffer, 0, MAX_LENGTH);

	char current_char;   // the current character that is reading
	int buffer_index = 0; // Input buffers index
	char *number;
	int first = 1;
	while ((current_char = fgetc(fd)) != EOF){
		if (current_char != '\n')
			buffer[buffer_index++] = current_char;
		else{
			/*Append to the list.*/
			if (!first){
				current_node->next = next;
				current_node = current_node->next;
			}
			/* Get the values of the sparse array from the buffer*/	
			number = strtok(buffer, "\t");
			if (number == NULL)
				err_exit("Error at finding next node.");

			current_node->cell.row = atoi(number);

			number = strtok(NULL, "\t");
			if (number == NULL)
				err_exit("Error at finding next node.");

			current_node->cell.col = atoi(number);
			
			current_node->cell.value = 1;
			
			current_node->next = NULL;

			/*Initialize the next element.*/
			if ((next = malloc(sizeof *next)) == NULL)
				err_exit("Malloc");

			/*Zero buffer.*/
			memset(buffer, 0, MAX_LENGTH);
			buffer_index = 0;
			first = 0;
		}
	}		

	if (fclose(fd) == EOF)
		err_exit("Error closing the file.");

	return data;
}


