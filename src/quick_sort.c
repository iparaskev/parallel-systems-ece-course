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
partition(int *arr, int *index, int low, int high, int *g_t)
{

	int pivot = arr[low];
	int lt = low;
	int gt = high - 1;
	int i = low;

	while(1)
	{
		if (arr[i] < pivot)
		{
			swap(&arr[i], &arr[lt], &index[i], &index[lt]);
			lt++;
			i++;
		}
		else if (arr[i] > pivot)
		{
			swap(&arr[i], &arr[gt], &index[i], &index[gt]);
			gt--;

		}
		else
			i++;

		if (i >= gt)
		{
			*g_t = gt;
			return lt;
		}

	}
}

void quick_sort(int *arr, int *index, int low, int high)
{
	if (low < high)
	{
		int gt;
		int lt = partition(arr, index, low, high, &gt);

		quick_sort(arr, index, low, lt - 1);
		quick_sort(arr, index, gt + 1, high);
	}
}
