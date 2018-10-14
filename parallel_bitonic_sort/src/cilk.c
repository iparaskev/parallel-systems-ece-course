#include <stdio.h>
#include <stdlib.h>
#include <cilk/cilk.h>
#include <sys/time.h>
#include "headers/cilk.h"

static const int ASCENDING = 1;
static const int DESCENDING = 0;
static int *a;

int __cilkrts_set_param(char *param, char *value);
void __cilkrts_end_cilk(void);
void __cilkrts_init(void);

// process of workers initiation
void setNWorkers(char *wokers){
    __cilkrts_end_cilk();
    __cilkrts_set_param("nworkers", wokers);
    __cilkrts_init();
}

// create array and initialize it for the cilk implementation 
void setACilk(int *b,int N)
{
	a = (int *) malloc(N * sizeof(int));
	a = b;
}

void exchangeCilk(int i,int j)
{
	int t;
    t = a[i];
    a[i] = a[j];
    a[j] = t;
}

void compareCilk(int i,int j,int dir)
{
	if (dir==(a[i]>a[j]))
		exchangeCilk(i,j);
}  

int cmprAscCilk(const void* a,const void* b){
  return ( *(int*)a - *(int*)b );
}

int cmprDescCilk(const void* a,const void* b){
  return ( - *(int*)a + *(int*)b );
}

void cilkBitonicMerge(int lo, int cnt, int dir) {
  if (cnt>1) {
    int k=cnt/2;
    int i;
    for (i=lo; i<lo+k; i++)
      compareCilk(i, i+k, dir);
    cilkBitonicMerge(lo, k, dir);
    cilkBitonicMerge(lo+k, k, dir);
  }
}



void cilkRecBitonicSort(int lo, int cnt, int dir) {
  if (cnt>1){
      int k=cnt/2;
      if (k > 512) {
        cilk_spawn cilkRecBitonicSort(lo, k, ASCENDING);
        cilkRecBitonicSort(lo+k, k, DESCENDING);
        cilk_sync;
        cilkBitonicMerge(lo, cnt, dir);
      }
      else {
        if (dir)  
          qsort(&a[lo],2*k,sizeof(int),cmprAscCilk);
        else 
          qsort(&a[lo],2*k,sizeof(int),cmprDescCilk);
      }
    }
}


/** function cilkSort()
    Caller of cilkRecBitonicSort for sorting the entire array of length N
    in ASCENDING order
**/
void cilkSort(int N,int n_thr) {
  // set number of threads
  char str[6];
  sprintf(str, "%d", n_thr);
  setNWorkers(str);

  cilkRecBitonicSort(0, N, ASCENDING);
}


void cilkImpBitonicSort(int N,int n_thr)
{	
  // set number of threads
  char str[6];
  sprintf(str, "%d", n_thr);  // convert int into char *
  setNWorkers(str);

	int i,j,k;
	for (k=2; k<=N; k=2*k) {
		for (j=k>>1; j>0; j=j>>1) {
			cilk_for (i=0; i<N; i++) {
				int ij=i^j;
				if ((ij)>i) 
				{
					if ((i&k)==0 && a[i] > a[ij]) 
						exchangeCilk(i,ij);
					if ((i&k)!=0 && a[i] < a[ij])
						exchangeCilk(i,ij);
				}
			}
		}
	}
}
