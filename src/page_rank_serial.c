#include "data_parser.h"
#include <stddef.h>
#include <stdio.h>

int 
main(int argc, char **argv)
{
	int N = 0;
	char *dataset_path = argv[1];
	Sparse *array = parse_data(dataset_path);	
	while (array != NULL){
		array = array->next;
		N++;
	}
	printf("%d\n", N);
	return 0;
}
