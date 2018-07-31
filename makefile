SHELL := /bin/bash

# VARIABLES
CC = gcc
RM = rm -f
RUN = page_rank_serial
_OBJ = page_rank_serial.o data_parser.o helper.o 
CFLAGS = -O3 -I 
_DEPS = sparse.h data_parser.h constants.h 

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
	$(CC) -c -o $@ $< $(CFLAGS)$(HEADERS) 

page_rank_serial: $(OBJ) 
	$(CC) -o $(BIN)/$@ $^

clean:
	$(RM) $(OBJD)/*

clean_all:
	$(RM) $(OBJD)/* $(BIN)/*

