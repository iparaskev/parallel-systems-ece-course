#include <stdio.h>
#include <stdlib.h>
#include <omp.h>
#include "headers/openmp.h"

static const int ASCENDING = 1;
static const int DESCENDING = 0;
static int *a;
static int active=0,max_threads;

// mutex key
omp_lock_t writelock;

// compare for qsort ascending order
int cmprAscOmp(const void* a,const void* b){
	return ( *(int*)a - *(int*)b );
}

int cmprDescOmp(const void* a,const void* b){
	return ( - *(int*)a + *(int*)b );
}

// compare for qsort descending order
void setAomp(int *b,int N)
{
	a = (int *) malloc(N * sizeof(int));
	a = b;
}

void exchangeOmp(int i,int j)
{
	int t;
    t = a[i];
    a[i] = a[j];
    a[j] = t;
}

void compareOmp(int i,int j,int dir)
{
	if (dir==(a[i]>a[j]))
		exchangeOmp(i,j);
}

void ompBitonicMerge(int lo,int cnt,int dir)
{
	if (cnt>1)
	{
	    int k=cnt/2;
	    int i;
	    for (i=lo; i<lo+k; i++)
	      compareOmp(i, i+k, dir);
	    ompBitonicMerge(lo, k, dir);
	    ompBitonicMerge(lo+k, k, dir);
    }
}

void ompRecBitonicSort(int lo,int cnt,int dir)
{
	if (cnt>1)
	{
		int k=cnt/2;
		// if number of elements is > than 512 do the parallel implementation
		// else use qsort for sorting
		if (k > 512 )
		{

			// if we haven't maxed our threads
			if (active < max_threads)
			{
				// lock the key to update max active threads one at a time
				omp_set_lock(&writelock);
				active += 2;
				omp_unset_lock(&writelock);

				omp_set_num_threads(2);
				#pragma omp parallel 
				{
					#pragma omp sections
					{	
						#pragma omp section
						{
							ompRecBitonicSort(lo, k, ASCENDING);
						}
						#pragma omp section
						{
							ompRecBitonicSort(lo+k, k, DESCENDING);
						}
					}
				}
				omp_set_lock(&writelock);
				active -= 2;
				omp_unset_lock(&writelock);

			}
			else
			{
				ompRecBitonicSort(lo, k, ASCENDING);
				ompRecBitonicSort(lo+k, k, DESCENDING);
			}
			ompBitonicMerge(lo, cnt, dir);	
		}
		else
		{
			
			if (dir)
			{
				qsort(&a[lo],2*k,sizeof(int),cmprAscOmp);
			}
			else
			{
				qsort(&a[lo],2*k,sizeof(int),cmprDescOmp);
			}
			
		}
  }
}

/** function sort()
    Caller of ompRecBitonicSort for sorting the entire array of length N
    in ASCENDING order
**/
void ompSort(int N,int n_thr)
{	
	omp_set_nested(1);
	max_threads = n_thr;
	// initialize mutex key
	omp_init_lock(&writelock);
	ompRecBitonicSort(0,N,ASCENDING);
}
void ompImpBitonicSort(int N,int n_thr)
{
	
	int i,j,k;
	for (k=2; k<=N; k=2*k) {
		for (j=k>>1; j>0; j=j>>1) {
			#pragma omp parallel for num_threads(n_thr)
			for (i=0; i<N; i++) {
				int ij=i^j;
				if ((ij)>i) 
				{
					if ((i&k)==0 && a[i] > a[ij]) 
						exchangeOmp(i,ij);
					if ((i&k)!=0 && a[i] < a[ij])
						exchangeOmp(i,ij);
				}
			}
		}
	}
}

