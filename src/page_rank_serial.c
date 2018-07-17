#include "data_parser.h"
#include <stddef.h>
#include <stdio.h>

int 
main(int argc, char **argv)
{
	int N;
	Sparse *res = parse_data(argv[1], &N);
	Sparse *array = res; 
	printf("%d\n", N);
	
	int b = 0;
	while (array != NULL){
		printf("%d %d \n", array->row, array->col);
		array = array->next;
		b++;
	}
	printf("%d\n", b);
	return 0;
}
