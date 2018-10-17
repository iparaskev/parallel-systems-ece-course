SHELL := /bin/bash

# VARIABLES
CC = gcc
MPI = mpicc
RM = rm -f
RUN = knn
MKDIR = mkdir -p
_OBJ = knn.o knn-solutions.o helper.o globals.o
CFLAGS = -fopenmp -O3  
_DEPS = helper.h globals.h knn-solutions.h
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
	$(MPI) -c -o $@ $< $(CFLAGS) -I$(HEADERS) 

knn: $(OBJ)
	$(MPI) -o $(BIN)/$@ $^ $(CFLAGS)

clean:
	$(RM) $(OBJD)/*

clean_all:
	$(RM) $(OBJD)/* $(BIN)/*





