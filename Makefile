all: addresser bsizer disker

addresser: src/addresser.c
	gcc -Wall -o bin/addresser.out src/addresser.c

bsizer: src/bsizer.c
	gcc -Wall -o bin/bsizer.out src/bsizer.c

disker: src/disker.c
	gcc -Wall -o bin/disker.out src/disker.c

clean:
	$(RM) bin/addresser.out bin/bsizer.out bin/disker.out
