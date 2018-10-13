#ifndef SPARSE_H_
#define SPARSE_H_

/* struct for a cell of a sparse array */
typedef struct Sparse_cell
{
        int row, col;
        float value;
}Sparse;

/* A linked list of sparse cellsi.*/
typedef struct Sparse_mod
{
        Sparse cell;
        struct Sparse_mod *next;
}Sparse_list;

/* Struct for saving the sparse to an array with known rows*/
typedef struct mid_cell
{
	int col;
	float value;
} Sparse_half;

#endif
