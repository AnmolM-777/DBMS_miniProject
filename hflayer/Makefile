CC = gcc
CFLAGS = -Wall -g -I. -I../pflayer

HF_OBJ = hf.o
PF_OBJ = ../pflayer/pflayer.o

all: hflayer.o testhf

hflayer.o: $(HF_OBJ)
	ld -r -o hflayer.o $(HF_OBJ)

testhf: testhf.o hflayer.o $(PF_OBJ)
	$(CC) $(CFLAGS) -o testhf testhf.o hflayer.o $(PF_OBJ)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f *.o testhf student hflayer.o hf_student_benchmark

hf_student_benchmark: hf_student_benchmark.c hflayer.o $(PF_OBJ)
	$(CC) $(CFLAGS) -o hf_student_benchmark hf_student_benchmark.c hflayer.o $(PF_OBJ)
