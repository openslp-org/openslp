# Painfully simple makefile for slpperf
CC = gcc

BINARY = slpperf
SOURCES = slp_linkedlist.c \
          slpperf.c \
          slpperf_test1.c

$(BINARY): $(SOURCES)
	$(CC) -o$(BINARY) -lslp $(SOURCES)
	
debug: $(SOURCES)
	$(CC) -g -o$(BINARY) -lslp $(SOURCES)

clean:
	rm -f *.o
	rm -f $(BINARY)
