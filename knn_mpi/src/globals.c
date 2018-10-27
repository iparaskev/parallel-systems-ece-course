#include "globals.h"

int rows, columns; // Rows and columns of the dataset's batch for every process.
int k;             // Number of neighbors.
int el_per_proc;   // How many elements has the process.
int files;
int rank;          // Process's rank
int world_size;    // The number of processes.
