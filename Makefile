all: addresser bsizer randomer

addresser: src/addresser.c
	gcc -Wall -o bin/addresser.out src/addresser.c

bsizer: src/bsizer.c
	gcc -Wall -o bin/bsizer.out src/bsizer.c

randomer: src/randomer.c
	gcc -Wall -pthread -o bin/randomer.out src/randomer.c

clean:
	$(RM) bin/addresser.out bin/bsizer.out bin/randomer.out
