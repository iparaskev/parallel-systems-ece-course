#include <stdio.h>
// A utility function to swap two elements
void swap(int* a, int* b, int* i, int *j)
{
    int t = *a;
    *a = *b;
    *b = t;

    t = *i;
    *i = *j;
    *j = t;
}
 
/* This function takes last element as pivot, places
   the pivot element at its correct position in sorted
    array, and places all smaller (smaller than pivot)
   to left of pivot and all greater elements to right
   of pivot */
int partition (int arr[], int index[], int low, int high)
{
    int pivot = arr[high];    // pivot
    int i = (low - 1);  // Index of smaller element
 
    for (int j = low; j <= high- 1; j++)
    {
        // If current element is smaller than or
        // equal to pivot
        if (arr[j] <= pivot)
        {
            i++;    // increment index of smaller element
            swap(&arr[i], &arr[j], &index[i], &index[j]);
        }
    }
    swap(&arr[i + 1], &arr[high], &index[i + 1], &index[high]);
    return (i + 1);
}
 
/* The main function that implements QuickSort
 arr[] --> Array to be sorted,
  low  --> Starting index,
  high  --> Ending index */
void quickSort(int arr[], int index[], int low, int high)
{
    if (low < high)
    {
        /* pi is partitioning index, arr[p] is now
           at right place */
        int pi = partition(arr, index, low, high);
 
        // Separately sort elements before
        // partition and after partition
        quickSort(arr, index, low, pi - 1);
        quickSort(arr, index, pi + 1, high);
    }
}
