# Painfully simple makefile for slptool

slptool:
	$(CC) -oslptool ${LIBDIR} -lslp slptool.c
	
debug:
	$(CC) -g -oslptool ${LIBDIR} -lslp slptool.c

clean:
	rm -f *.o
	rm -f slptool
