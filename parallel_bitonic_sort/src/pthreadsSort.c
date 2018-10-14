#include "headers/pthreadsSort.h"
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

static int *a;
static int thread_size;
static int num_threads;
static const int ASCENDING = 1;
static const int DESCENDING = 0;
pthread_mutex_t mutex;
pthread_attr_t attrRec;
int active=0;

// compare for qsort ascending order
int cmprAscPthr(const void* a,const void* b){
	return ( *(int*)a - *(int*)b );
}

// compare for qsort descending order
int cmprDescPthr(const void* a,const void* b){
	return ( - *(int*)a + *(int*)b );
}

// imperative sort attributes
typedef struct 
{
	int id,pace,k;
}parm;

// recersive sort attributes
typedef struct 
{
	int lo,cnt,dir;
}rec_param;

void setApthr(int *b,int N)
{
	a = (int *) malloc(N * sizeof(int));
	a = b;
}

void exchangePthr(int i,int j)
{
	int t;
    t = a[i];
    a[i] = a[j];
    a[j] = t;
}

void comparePthr(int i,int j,int dir)
{
	if (dir==(a[i]>a[j]))
		exchangePthr(i,j);
}

void pthrBitonicMerge(int lo,int cnt,int dir)
{
	if (cnt>1)
	{
	    int k=cnt/2;
	    int i;
	    for (i=lo; i<lo+k; i++)
	      comparePthr(i, i+k, dir);
	    pthrBitonicMerge(lo, k, dir);
	    pthrBitonicMerge(lo+k, k, dir);
    }
}

/* function pthrRecBitonicSort
	 is the parallel implementation of recBitonicSort
	 if the active threads are fewer than the thread limits
	 creates two threads and recursively calls itself, else
	 does the same but at the same thread that has call it.
*/
void *pthrRecBitonicSort(void *par)
{
	int parallel=0;

	// typecast attributes back to rec_param
	rec_param params = *(rec_param *) par;
	int lo = params.lo;
	int cnt = params.cnt;
	int dir = params.dir;

	int k=cnt/2;

	if (cnt>1){
		if (k > 512)
		{
			if (active < num_threads){

				// lock mutex to update active
				pthread_mutex_lock(&mutex);
				active += 2;
				pthread_mutex_unlock(&mutex);
				parallel=1;
			}
			if (parallel){
				
				pthread_t thread1,thread2;
				rec_param thr,thr2;
				
				// thread1 attributes
				thr.lo = lo;
				thr.cnt = k;
				thr.dir = ASCENDING;
				pthread_create(&thread1,&attrRec,pthrRecBitonicSort,&thr);
				
				//thread2 attributes
				thr2.lo = lo+k;
				thr2.cnt = k;
				thr2.dir = DESCENDING;
				pthread_create(&thread2,&attrRec,pthrRecBitonicSort,&thr2);
				
				// wait for threads to finish
				pthread_join(thread1,NULL);
				pthread_join(thread2,NULL);

				// update active
				pthread_mutex_lock(&mutex);
				active -= 2;
				pthread_mutex_unlock(&mutex);
			}
			else
			{
				rec_param thr,thr2;
				thr.lo = lo;
				thr.cnt = k;
				thr.dir = ASCENDING;
				
				pthrRecBitonicSort(&thr);
				thr2.lo = lo+k;
				thr2.cnt = k;
				thr2.dir = DESCENDING;
				pthrRecBitonicSort(&thr2);
				
			}
			pthrBitonicMerge(lo,cnt,dir);
		}
		else
		{
			if (dir)
			{
				qsort(&a[lo],2*k,sizeof(int),cmprAscPthr);
			}
			else
			{
				qsort(&a[lo],2*k,sizeof(int),cmprDescPthr);
			}
		}
	}
}

/** function sort() 
   Caller of pthrRecBitonicSort for sorting the entire array of length N 
   in ASCENDING order
 **/
void pthrSort(int N,int n_thr)
{
	num_threads = n_thr;
	// initialize mutex and attr
	pthread_mutex_init(&mutex,NULL);
	pthread_attr_init(&attrRec);
	pthread_attr_setdetachstate(&attrRec,PTHREAD_CREATE_JOINABLE);
	pthread_t thread;
	
	rec_param params;
	params.lo=0;
	params.cnt=N;
	params.dir=ASCENDING;

	active+=1;
	
	pthread_create(&thread,&attrRec,pthrRecBitonicSort,&params);
	pthread_join(thread,NULL);
}

void *runEl(void *atr)
{
	// typecast the atribute back to the struct
	parm *at = (parm *) atr;
	int thread_id = at->id;
	int start = thread_id*thread_size;
	int end = start + thread_size;
	int j = at->pace;
	int k = at->k;

	for (int i=start; i<end; i++) {
		int ij=i^j;
		if ((ij)>i) {
			if ((i&k)==0 && a[i] > a[ij]) 
				exchangePthr(i,ij);
			if ((i&k)!=0 && a[i] < a[ij])
				exchangePthr(i,ij);
		}
	}
}


void pthrImpBitonicSort(int N,int n_thr)
{	
	
	int i,j,k;
	/*define number of threads */
	pthread_t threads[n_thr];
	pthread_attr_t attr;
	// thread's function atributes stuct
	parm *threadAtr;
	threadAtr=(parm *)malloc(sizeof(parm)*n_thr);
	//threadAtr = (parm *)malloc(sizeof(parm)*n_thr);

	// initialize pthread attribute 
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

	// how many elements each thread will have 
	thread_size = (N/n_thr);
	for (k=2; k<=N; k=2*k) {

		for (j=k>>1; j>0; j=j>>1) {
			// change the pace for the comparisons
			
			for (i=0;i<n_thr;i++){
				threadAtr[i].k = k;
				threadAtr[i].pace=j; 
				threadAtr[i].id = i;
				pthread_create(&threads[i],&attr,runEl,(void *) &threadAtr[i]);
			}
			for (i=0;i<n_thr;i++){
				pthread_join(threads[i],NULL);
			}
		}
	}

	// delete attribute
	pthread_attr_destroy(&attr);
}