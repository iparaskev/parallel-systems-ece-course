#ifndef CILK_H_
#define CILK_H_

void setACilk(int *b,int N);
void exchangeCilk(int i,int j);
void compareCilk(int i,int j,int dir);
void cilkRecBitonicSort(int lo,int cnt,int dir);
void cilkBitonicMerge(int lo,int cnt,int dir);
void cilkSort(int N,int n_thr);
void cilkImpBitonicSort(int N,int n_thr);
void setNWorkers(char *wokers);

int cmprAscCilk(const void* a,const void* b);
int cmprDescCilk(const void* a,const void* b);
#endif