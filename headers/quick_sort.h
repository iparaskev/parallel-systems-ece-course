#ifndef QUICKSORT_H
#define QUICKSORT_H

void swap(int* a, int* b, int* i, int *j);
int partition (int arr[], int index[], int low, int high);
void quickSort(int arr[], int index[], int low, int high);

#endif
