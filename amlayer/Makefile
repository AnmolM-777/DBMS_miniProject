CC = gcc
CFLAGS = -Wall -g -I. -I../pflayer

AM_OBJ = am.o amfns.o amsearch.o aminsert.o amstack.o amglobals.o amscan.o amprint.o
PF_OBJ = ../pflayer/pflayer.o

all: amlayer.o test1 test2 test3

amlayer.o: $(AM_OBJ)
	ld -r -o amlayer.o $(AM_OBJ)

test1: test1.o amlayer.o $(PF_OBJ)
	$(CC) $(CFLAGS) -o test1 test1.o amlayer.o $(PF_OBJ)

test2: test2.o amlayer.o $(PF_OBJ)
	$(CC) $(CFLAGS) -o test2 test2.o amlayer.o $(PF_OBJ)

test3: test3.o amlayer.o $(PF_OBJ)
	$(CC) $(CFLAGS) -o test3 test3.o amlayer.o $(PF_OBJ)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f *.o test1 test2 test3 am_student_benchmark testrel.* amlayer.o

am_student_benchmark: am_student_benchmark.c amlayer.o ../pflayer/pflayer.o
	$(CC) $(CFLAGS) -o am_student_benchmark am_student_benchmark.c amlayer.o ../pflayer/pflayer.o
