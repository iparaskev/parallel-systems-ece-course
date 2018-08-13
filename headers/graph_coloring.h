#include "list.h"
#include "sparse.h"

#ifndef GRAPH_COLORING_H
#define GRAPH_COLORING_H

list* make_undirected(Sparse_list *data);
int* coloring(list *graph);
Sparse_half** partitions(Sparse_half **A, int *color);

#endif
