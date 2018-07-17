#include "data_parser.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define MAX_LENGTH 200

Sparse* 
parse_data(char *filename)
{
	/*Initialize the datastructure.*/
	Sparse *data, *next;
	if ((data = malloc(sizeof *data)) == NULL){
		perror("Malloc");
		exit(1);
	}
	Sparse *current_node;
	current_node = data;

	/*Read file.*/
	FILE *fd;
	if ((fd = fopen(filename, "r")) == NULL){
		fprintf(stderr, "Error opening the dataset file.");
		exit(1);
	}
	
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
			current_node->row = atoi(number);

			number = strtok(NULL, "\t");
			current_node->col = atoi(number);
			
			current_node->value = 1;
			
			/*Zero buffer.*/
			memset(buffer, 0, MAX_LENGTH);
			buffer_index = 0;
			first = 0;

			/*Initialize the next element.*/
			if ((next = malloc(sizeof *next)) == NULL){
				perror("Malloc");
				exit(1);
			}
			current_node->next = NULL;
		}
	}		

	if (fclose(fd) == EOF){
		fprintf(stderr, "Error closing the file.\n");
		exit(1);
	}

	return data;
}


