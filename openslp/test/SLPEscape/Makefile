CC		= gcc
NAME	= SLPEscape
OBJS	= SLPEscape.o
CFLAGS	= -Wall -ggdb3 -DDEBUG
LIBPATH	= ../../libslp
INCPATH = ../
LIBS	= slp

.SUFFIXES: .c .o

%.o:	%.c
	$(CC) -I$(INCPATH) $(CFLAGS) -c $< 

all: $(OBJS)
	$(CC) -o $(NAME) $(OBJS) -L$(LIBPATH) -l$(LIBS) 

test:
	./test.script

clean:
	rm *.o core SLPEscape.actual.output $(NAME) -f
