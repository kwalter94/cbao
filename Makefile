CC=gcc
CFLAGS=-Wall -g3
LDFLAGS=
OBJ=tree.o error.o eval.o
TESTS=treeTest

bao: $(OBJ) main.c
	$(CC) $(CFLAGS) -o main $^

tree.o: tree.h tree.c
	$(CC) $(CFLAGS) -c tree.c

eval.o: tree.h tree.c eval.h eval.c
	$(CC) $(CFLAGS) -c eval.c

error.o: error.h error.c
	$(CC) $(CFLAGS) -c error.c

tests: $(TESTS)

clean:
	rm -vf *.o $(TESTS)
