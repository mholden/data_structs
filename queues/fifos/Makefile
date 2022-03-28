CC=gcc
CFLAGS=-g
DS=/home/mholden/devel/data_structs
INCLUDES=-I$(DS)/include

all: test_fifo

test_fifo: test_fifo.c fifo.c $(DS)/include/fifo.h
	$(CC) $(CFLAGS) $(INCLUDES) test_fifo.c fifo.c -o test_fifo

clean:
	rm *o test_fifo

.PHONY: all clean
