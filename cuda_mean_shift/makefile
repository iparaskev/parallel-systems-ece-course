SHELL :=/bin/bash

#=================

CC=gcc
NV=nvcc
RM=rm -f
CFLAGS= -lm -O3
NVFLAGS=-g -O4 -lm -lcuda -lcudart
RUN=mean_shift_cpu mean_shift_cuda mean_shift_cuda_shared

all: $(RUN)

mean_shift_cpu: mean_shift_cpu.c
	$(CC) -o $@ $^ $(CFLAGS)

mean_shift_cuda: mean_shift_cuda.cu
	$(NV) -o $@ $^ 

mean_shift_cuda_shared: mean_shift_cuda_shared.cu
	$(NV) -o $@ $^ 

clean: 
	$(RM) $(RUN) ~*
