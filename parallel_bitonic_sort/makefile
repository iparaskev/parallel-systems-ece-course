SHELL :=/bin/bash

#================================
#COMMANDS

HEAD=headers
CC=gcc
RM=rm -f
CFLAGS=-I. -fopenmp -fcilkplus -O2
_DEPS=simpleBitonic.h openmp.h pthreadsSort.h cilk.h
DEPS=$(patsubst %,$(HEAD)/%,$(_DEPS))
OBJ=testAll.o simpleBitonic.o openmp.o pthreadsSort.o cilk.o

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

test: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS)

clean:
	$(RM) *.o *~
