SHELL :=/bin/bash

#=======================
#COMMANDS

CC=gcc
MPI=mpicc
RM=rm -f
CFLAGS=-fopenmp -O3 
RUN =  knn-mpiblocking knn-mpinonblocking knn-serial

all: $(RUN)

knn-mpiblocking: knn-mpiblocking.c
	$(MPI) -o $@ $^ $(CFLAGS) 

knn-mpinonblocking: knn-mpinonblocking.c
	$(MPI) -o $@ $^ $(CFLAGS)

knn-serial: knn-serial-testing.c
	$(CC) -o $@ $^ $(CFLAGS)

clean:
	$(RM) $(RUN)