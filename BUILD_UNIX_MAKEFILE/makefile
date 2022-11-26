CC=gcc
CPP=g++
CFLAGS=-O3 -c -Wall

all: hello hello2 helloa autotest1

hello: hello.o iuliia.o
	$(CPP) hello.o iuliia.o -o hello

hello2: hello2.o iuliia.o
	$(CPP) hello2.o iuliia.o -o hello2

helloa: helloa.o iuliia.o
	$(CPP) helloa.o iuliia.o -o helloa

autotest1: autotest1.o iuliia.o
	$(CPP) autotest1.o iuliia.o -o autotest1
	
hello.o:
	$(CC) $(CFLAGS) ../hello.c

hello2.o:
	$(CC) $(CFLAGS) ../hello2.c

helloa.o:
	$(CC) $(CFLAGS) ../helloa.c

autotest1.o:
	$(CC) $(CFLAGS) ../autotest1.c

iuliia.o:
	$(CC) $(CFLAGS) ../iuliia.c
	
clean:
	rm -f *.o test
