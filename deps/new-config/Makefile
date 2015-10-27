LEX=flex
CC=gcc
CFLAGS=-g -std=gnu89 -pedantic -O2 -Wall -W -fPIC

all: libconfig.a

%.o : %.c
	$(CC) -c $(CFLAGS) -o $@ $<

lexer.c : lexer.l
	$(LEX) -o $@ $<

test: lexer.o config.o test.o
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^

libconfig.a: lexer.o config.o
	ar rcs $@ $^

clean:
	rm -f *.o

distclean: clean
	rm -f test
	rm -f libconfig.a
	rm lexer.c
