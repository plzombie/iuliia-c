CC=gcc
CPP=g++
CFLAGS=-O3 -c -Wall

all: hello hello2 helloa autotest1 autotest2 iuliia-c

hello: hello.o iuliia.o
	$(CPP) hello.o iuliia.o -o hello

hello2: hello2.o iuliia.o
	$(CPP) hello2.o iuliia.o -o hello2

helloa: helloa.o iuliia.o
	$(CPP) helloa.o iuliia.o -o helloa

autotest1: autotest1.o iuliia.o
	$(CPP) autotest1.o iuliia.o -o autotest1

autotest2: autotest2.o iuliia.o
	$(CPP) autotest2.o iuliia.o -o autotest2

iuliia-c: iuliia.o iuliia-c-frontend.o
	$(CPP) iuliia-c-frontend.o iuliia.o -o iuliia-c
	
hello.o: ../hello.c
	$(CC) $(CFLAGS) ../hello.c

hello2.o: ../hello2.c
	$(CC) $(CFLAGS) ../hello2.c

helloa.o: ../helloa.c
	$(CC) $(CFLAGS) ../helloa.c

autotest1.o: ../autotest1.c
	$(CC) $(CFLAGS) ../autotest1.c

autotest2.o: ../autotest2.c
	$(CC) $(CFLAGS) ../autotest2.c

iuliia.o: ../iuliia.c
	$(CC) $(CFLAGS) ../iuliia.c

iuliia-c-frontend.o: ../iuliia-c-frontend.c
	$(CC) $(CFLAGS) ../iuliia-c-frontend.c
	
clean:
	rm -f *.o hello hello2 helloa autotest1 iuliia-c
