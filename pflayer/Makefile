CC = gcc
CFLAGS = -Wall -g

SRC = buf.c hash.c pf.c
OBJ = buf.o hash.o pf.o
HDR = pftypes.h pf.h 

all: pflayer.o

pflayer.o: $(OBJ)
	ld -r -o pflayer.o $(OBJ)

%.o: %.c $(HDR)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f *.o pf_benchmark

pf_benchmark: pf_benchmark.c pflayer.o
	$(CC) $(CFLAGS) -o pf_benchmark pf_benchmark.c pflayer.o
