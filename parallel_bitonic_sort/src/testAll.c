#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "headers/simpleBitonic.h"
#include "headers/openmp.h"
#include "headers/pthreadsSort.h"
#include "headers/cilk.h"
#include <sys/time.h>

struct timeval startwtime, endwtime;
double seq_time;
static int *a;



void init(int N);
void test(int N);
void callFunc(int N,int n_thr,void (*f)(int,int),char* name);
void writeToFile(char **argv,int N,double seq_time,int n_thr);

int main(int argc,char** argv)
{

	//check for right arguments
	if (argc < 2){
		printf("Usage: %s q p\nWhere n=2^q array elements and n2=2^p threads\n"
			,argv[0]);
		exit(1);
	}

	int N;
	N = 1<<atoi(argv[1]);
	int n_thr;
	n_thr = 1<<atoi(argv[2]);
	char name[100];

	a = (int *) malloc(N * sizeof(int));

	

	// // sort with classic
	// strcpy(name,"Imperative serial");
	// callFunc(N,n_thr,impBitonicSort, name);

	// strcpy(name,"Recursive serial");
	// callFunc(N,n_thr,sort,name);
	

	// // sort with openmp
	// strcpy(name,"Imperative parallel omp");
	// callFunc(N,n_thr,ompImpBitonicSort, name);
	
	// strcpy(name,"Recursive parallel omp");
	// callFunc(N,n_thr,ompSort,name);

	// // sort with cilk
	// strcpy(name,"Imperative parallel cilk");
	// callFunc(N, n_thr, cilkImpBitonicSort, name);

	// strcpy(name,"Recursive parallel cilk");
	// callFunc(N, n_thr, cilkSort, name);

	// // sort with pthreads
	// strcpy(name,"Imperative parallel pthreads");
	// callFunc(N, n_thr, pthrImpBitonicSort, name);

	strcpy(name,"Recursive parallel pthreads");
	callFunc(N, n_thr, pthrSort, name);
	
	
	// // sort with qsort
	// init(N);
	// gettimeofday (&startwtime, NULL);
	// qsort(a,N,sizeof(int),cmprAsc);
	// gettimeofday (&endwtime, NULL);
	// seq_time = (double)((endwtime.tv_usec - startwtime.tv_usec)/1.0e6
	// 	      + endwtime.tv_sec - startwtime.tv_sec);
	// printf("%s wall clock time = %f\n","qsort", seq_time);	
	
	// writeToFile(argv,N,seq_time,n_thr);
	
	return 0;

}

void init(int N)
{
	for(int i=0;i<N;i++){
		a[i] = rand() % N;
	}
}

void test(int N)
{
	int pass = 1;
	int i;
	for (i = 1; i < N; i++) {
		pass &= (a[i-1] <= a[i]);
	}

	printf(" TEST %s\n",(pass) ? "PASSed" : "FAILed");
}

void callFunc(int N,int n_thr,void (*f)(int,int),char* name)
{
	init(N);
	setA(a,N);
	setAomp(a,N);
	setACilk(a,N);
	setApthr(a,N);
	gettimeofday (&startwtime, NULL);
	f(N,n_thr);
	gettimeofday (&endwtime, NULL);
	seq_time = (double)((endwtime.tv_usec - startwtime.tv_usec)/1.0e6
		      + endwtime.tv_sec - startwtime.tv_sec);
	printf("%s wall clock time = %f\n",name, seq_time);	
	// test(N);
}

void writeToFile(char** argv,int N,double seq_time,int n_thr)
{
	// make N string 
	char *folder = "txts/";
	char *prog_name = (char *)malloc(strlen(folder) + strlen(argv[3]) + 1);
	strcpy(prog_name,folder);
	strcat(prog_name,argv[3]);
	char *format=".txt";
	char *filename = (char *)malloc(strlen(prog_name)+strlen(argv[1])+strlen(format) +1 );

	/* copy the value of first string and then 
	  concatenate */
	strcpy(filename,prog_name);
	strcat(filename,argv[1]);
	strcat(filename,format);
	

	// open and write to file
	FILE *f = fopen(filename,"a");
	if (f==NULL){
		printf("Error opening the file\n");
		exit(1);
	}

	fprintf(f, "%d %f\n",n_thr,seq_time );
	fclose(f);
	printf("Wrote to %s for threads %d and time %f\n",filename,n_thr,seq_time );
}
