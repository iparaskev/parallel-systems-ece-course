SHELL := /bin/bash

# VARIABLES
CC = gcc
RM = rm -f
RUN = page_rank_serial
_OBJ = page_rank_serial.o data_parser.o
CFLAGS = -I 
_DEPS = sparse.h data_parser.h  

# DIRECTORIES
SRC = src
BIN = bin
HEADERS = headers
OBJD = obj

DEPS = $(patsubst %, $(HEADERS)/%,$(_DEPS))
OBJ = $(patsubst %, $(OBJD)/%,$(_OBJ))

# COMMANDS
$(OBJD)/%.o: $(SRC)/%.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)$(HEADERS) 

all: $(RUN)

page_rank_serial: $(OBJ) 
	$(CC) -o $(BIN)/$@ $^

clean:
	$(RM) $(OBJD)/*

clean all:
	$(RM) $(OBJD)/* $(BIN)/*

