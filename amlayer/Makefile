CC ?= gcc
CFLAGS ?= -std=gnu89 -O2 -Wno-implicit-function-declaration -Wno-return-type
.PHONY: all clean
all: amlayer.o
amlayer.o:
	$(MAKE) -C .. am
	cp ../amlayer/amlayer.o ./amlayer.o
clean:
	rm -f *.o
