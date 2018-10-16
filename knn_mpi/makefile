SHELL := /bin/bash

# VARIABLES
CC = gcc
MPI = mpicc
RM = rm -f
RUN = knn-mpiblocking knn-mpinonblocking knn-serial
MKDIR = mkdir -p
_OBJ = knn-mpiblocking.o knn-mpinonblocking.o knn-serial.o helper.o globals.o
CFLAGS = -fopenmp -O3  
_DEPS = helper.h globals.h
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

knn-mpiblocking: $(OBJ)
	$(MPI) -o $@ $^ $(CFLAGS) 

knn-mpinonblocking: $(OBJ)
	$(MPI) -o $@ $^ $(CFLAGS)

knn-serial: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS)

clean:
	$(RM) $(OBJD)/*

clean_all:
	$(RM) $(OBJD)/* $(BIN)/*





