#ifndef PAGERANK_PARALLEL_H
#define PAGERANK_PARALLEL_H

double *pagerank_par(Sparse_half **adjacency, int rows, int *row_sums);

#endif
