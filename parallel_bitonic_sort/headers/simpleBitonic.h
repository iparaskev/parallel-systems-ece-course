#ifndef SIMPLEBITONIC_H_
#define SIMPLEBITONIC_H_

void setA(int *b,int N);
void exchange(int i,int j);
void compare(int i,int j,int dir);
void bitonicMerge(int lo,int cnt,int dir);
void recBitonicSort(int lo, int cnt, int dir);
void impBitonicSort(int N,int n_thr);
void sort(int N,int n_thr);

int cmprAsc(const void* a,const void* b);
int cmprDesc(const void* a,const void* b);


#endif
