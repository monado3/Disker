CC = gcc
CFLAGS = -Wall

all: addresser bsizer randomer

addresser: mylib.o addresser.o
	$(CC) $(CFLAGS) -o bin/addresser.out bin/mylib.o bin/addresser.o

bsizer: mylib.o bsizer.o
	$(CC) $(CFLAGS) -o bin/bsizer.out bin/mylib.o bin/bsizer.o

randomer: mylib.o randomer.o
	$(CC) $(CFLAGS) -pthread -o bin/randomer.out bin/mylib.o bin/randomer.o

mylib.o : src/mylib.c
	$(CC) $(CFLAGS) -c src/mylib.c -o bin/mylib.o

addresser.o : src/addresser.c
	$(CC) $(CFLAGS) -c src/addresser.c -o bin/addresser.o

bsizer.o : src/bsizer.c
	$(CC) $(CFLAGS) -c src/bsizer.c -o bin/bsizer.o

randomer.o : src/randomer.c
	$(CC) $(CFLAGS) -c src/randomer.c -o bin/randomer.o

clean:
	$(RM) bin/*.out bin/*.o
