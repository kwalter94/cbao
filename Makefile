CC=gcc
CFLAGS=-Wall -g3
LDFLAGS=
OBJ=tree.o error.o
TESTS=treeTest

bao: $(OBJ) main.c
	$(CC) $(CFLAGS) -o main $^

tree.o: tree.h tree.c
	$(CC) $(CFLAGS) -c tree.c

error.o: error.h error.c
	$(CC) $(CFLAGS) -c error.c

tests: $(TESTS)

clean:
	rm -vf *.o $(TESTS)
