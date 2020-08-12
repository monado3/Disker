CC = gcc
CFLAGS = -Wall -msse2 -DSFMT_MEXP=19937

all: addresser bsizer randomer

addresser: SFMT.o mylib.o addresser.o
	$(CC) $(CFLAGS) -o bin/addresser.out bin/mylib.o bin/SFMT.o bin/addresser.o

bsizer: SFMT.o mylib.o bsizer.o
	$(CC) $(CFLAGS) -o bin/bsizer.out bin/mylib.o bin/SFMT.o bin/bsizer.o

randomer: SFMT.o mylib.o randomer.o
	$(CC) $(CFLAGS) -pthread -o bin/randomer.out bin/mylib.o bin/SFMT.o bin/randomer.o

mylib.o : src/mylib.c
	$(CC) $(CFLAGS) -c src/mylib.c -o bin/mylib.o

addresser.o : src/addresser.c
	$(CC) $(CFLAGS) -c src/addresser.c -o bin/addresser.o

bsizer.o : src/bsizer.c
	$(CC) $(CFLAGS) -c src/bsizer.c -o bin/bsizer.o

randomer.o : src/randomer.c
	$(CC) $(CFLAGS) -c src/randomer.c -o bin/randomer.o

SFMT.o : src/SFMT.c
	$(CC) $(CFLAGS) -c src/SFMT.c -o bin/SFMT.o

clean:
	$(RM) bin/*.out bin/*.o
