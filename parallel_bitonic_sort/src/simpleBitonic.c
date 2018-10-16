#include <stdio.h>
#include <stdlib.h>
#include "simpleBitonic.h"

const int ASCENDING = 1;
const int DESCENDING = 0;
static int *a;

int cmprAsc(const void* a,const void* b){
	return ( *(int*)a - *(int*)b );
}

int cmprDesc(const void* a,const void* b){
	return ( - *(int*)a + *(int*)b );
}

void setA(int *b,int N)
{
	a = (int *) malloc(N * sizeof(int));
	a = b;
}

void exchange(int i,int j)
{
	int t;
    t = a[i];
    a[i] = a[j];
    a[j] = t;
}

void compare(int i,int j,int dir)
{
	if (dir==(a[i]>a[j]))
		exchange(i,j);
}

void bitonicMerge(int lo,int cnt,int dir)
{
	if (cnt>1)
	{
	    int k=cnt/2;
	    int i;
	    for (i=lo; i<lo+k; i++)
	      compare(i, i+k, dir);
	    bitonicMerge(lo, k, dir);
	    bitonicMerge(lo+k, k, dir);
    }
}

void recBitonicSort(int lo, int cnt, int dir)
{
	if (cnt>1)
	{
	    int k=cnt/2;
	    recBitonicSort(lo, k, ASCENDING);
	    recBitonicSort(lo+k, k, DESCENDING);
	    bitonicMerge(lo, cnt, dir);
    }
}

void impBitonicSort(int N,int n_thr)
{	
	
	int i,j,k;
	for (k=2; k<=N; k=2*k) {
		for (j=k>>1; j>0; j=j>>1) {
			for (i=0; i<N; i++) {
				int ij=i^j;
				if ((ij)>i) 
				{
					if ((i&k)==0 && a[i] > a[ij]) 
						exchange(i,ij);
					if ((i&k)!=0 && a[i] < a[ij])
						exchange(i,ij);
				}
			}
		}
	}
}

void sort(int N,int n_thr)
{
	recBitonicSort(0, N, ASCENDING);
}
