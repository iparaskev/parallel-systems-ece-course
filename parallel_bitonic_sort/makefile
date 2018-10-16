SHELL := /bin/bash

# VARIABLES
CC = gcc
RM = rm -f
RUN = test
MKDIR = mkdir -p
_OBJ = testAll.o simpleBitonic.o openmp.o pthreadsSort.o cilk.o
CFLAGS = -fopenmp -fcilkplus -pthread -O3 
_DEPS = simpleBitonic.h openmp.h pthreadsSort.h cilk.h
DIRS = bin obj

# DIRECTORIES
SRC = src
BIN = bin
HEADERS = headers
OBJD = obj

DEPS = $(patsubst %, $(HEADERS)/%,$(_DEPS))
OBJ = $(patsubst %, $(OBJD)/%,$(_OBJ))

# COMMANDS
all: $(DIRS) $(RUN)

$(DIRS):
	$(MKDIR) $@

$(OBJD)/%.o: $(SRC)/%.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS) -I$(HEADERS) 

test: $(OBJ) 
	$(CC) -o $(BIN)/$@ $^ $(CFLAGS)

clean:
	$(RM) $(OBJD)/*

clean_all:
	$(RM) $(OBJD)/* $(BIN)/*
