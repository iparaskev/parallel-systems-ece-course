#ifndef PTHREADSSORT_H_
#define PTHREADSSORT_H_

void setApthr(int *b,int N);
void exchangePthr(int i,int j);
void *runEl(void *atr);
void comparePthr(int i,int j,int dir);
void *pthrRecBitonicSort(void *par);
void pthrBitonicMerge(int lo,int cnt,int dir);
void pthrImpBitonicSort(int N,int n_thr);
void pthrSort(int N,int n_thr);

int cmprAscPthr(const void* a,const void* b);
int cmprDescPthr(const void* a,const void* b);
#endif
