#ifndef SPARSE_H_
#define SPARSE_H_

typedef struct Sparse_mod
{
        int row, col;
        int value;
        struct Sparse_mod *next;
}Sparse;

#endif
