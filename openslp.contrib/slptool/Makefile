# Painfully simple makefile for slptool

message:
	cat README
	
slptool: slptool.c
	$(CC) -oslptool ${LIBS} ${INCS} -lslp slptool.c

debug:
	$(CC) -g -oslptool ${LIBS} ${INCS} -lslp slptool.c

clean:
	rm -f *.o
	rm -f slptool
