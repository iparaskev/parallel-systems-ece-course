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

int
partition(int *arr, int *index, int low, int high)
{
	int pivot = arr[low];
	int i = low - 1;
	int j = high + 1;

	while(1)
	{
		do
			i++;
		while (arr[i] < pivot);

		do
			j--;
		while (arr[j] > pivot);

		if (i >= j)
			return j;

		swap(&arr[i], &arr[j], &index[i], &index[j]);
	}
}

void quick_sort(int *arr, int *index, int low, int high)
{
	if (low < high)
	{
		int pivot_index = partition(arr, index, low, high);

		quick_sort(arr, index, low, pivot_index);
		quick_sort(arr, index, pivot_index + 1, high);
	}
}
