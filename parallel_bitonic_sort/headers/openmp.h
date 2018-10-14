#ifndef OPENMP_H_
#define OPENMP_H_

void setAomp(int *b,int N);
void exchangeOmp(int i,int j);
void compareOmp(int i,int j,int dir);
void ompRecBitonicSort(int lo,int cnt,int dir);
void ompBitonicMerge(int lo,int cnt,int dir);
void ompSort(int N,int n_thr);
void ompImpBitonicSort(int N,int n_thr);

int cmprAscOmp(const void* a,const void* b);
int cmprDescOmp(const void* a,const void* b);

#endif