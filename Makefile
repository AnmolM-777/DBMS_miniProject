CC ?= gcc
CFLAGS ?= -std=gnu89 -O2 -Wall -Wextra -Wno-implicit-function-declaration
CPPFLAGS ?= -I./pflayer -I./amlayer

PF_OBJECTS = pflayer/buf.o pflayer/hash.o pflayer/pf.o
AM_SOURCES = am.c amfns.c amsearch.c aminsert.c amstack.c amglobals.c \
             amscan.c amprint.c bulkload.c
AM_OBJECTS = $(AM_SOURCES:%.c=amlayer/%.o)

.PHONY: all pf workload am clean

all: pf workload am

pf: pflayer/pflayer.o

pflayer/pflayer.o: $(PF_OBJECTS)
	ld -r -o $@ $^

workload: experiments/pf_workload

experiments/pf_workload: experiments/pf_workload.c pflayer/pflayer.o
	$(CC) $(CFLAGS) $(CPPFLAGS) -o $@ $^

am: amlayer/amlayer.o

amlayer/amlayer.o: $(AM_OBJECTS)
	ld -r -o $@ $^

pflayer/%.o: pflayer/%.c pflayer/pf.h pflayer/pftypes.h
	$(CC) $(CFLAGS) $(CPPFLAGS) -c -o $@ $<

amlayer/%.o: amlayer/%.c amlayer/am.h amlayer/pf.h
	$(CC) $(CFLAGS) $(CPPFLAGS) -c -o $@ $<

clean:
	rm -f $(PF_OBJECTS) pflayer/pflayer.o $(AM_OBJECTS) amlayer/amlayer.o \
		experiments/pf_workload
