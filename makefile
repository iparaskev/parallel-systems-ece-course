SHELL := /bin/bash

# VARIABLES
CC = gcc
RM = rm -f
RUN = pagerank
_OBJ = pagerank.o page_rank_serial.o data_parser.o helper.o pagerank_parallel.o graph_coloring.o list.o globals.o quick_sort.o
CFLAGS = -O3 -I 
_DEPS = sparse.h data_parser.h constants.h page_rank_serial.h pagerank_parallel.h helper.h list.h globals.h quick_sort.h

# DIRECTORIES
SRC = src
BIN = bin
HEADERS = headers
OBJD = obj

DEPS = $(patsubst %, $(HEADERS)/%,$(_DEPS))
OBJ = $(patsubst %, $(OBJD)/%,$(_OBJ))

# COMMANDS
all: $(RUN)

$(OBJD)/%.o: $(SRC)/%.c $(DEPS)
	$(CC) -c -fopenmp -o $@ $< $(CFLAGS)$(HEADERS) 

pagerank: $(OBJ) 
	$(CC) -pthread -o $(BIN)/$@ $^

clean:
	$(RM) $(OBJD)/*

clean_all:
	$(RM) $(OBJD)/* $(BIN)/*

