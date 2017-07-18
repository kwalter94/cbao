CC=gcc
CFLAGS=-Wall -g3
LDFLAGS=
OBJ=BaoTree.o BaoError.o
TESTS=BaoTreeTest

BaoTree.o: BaoTree.h BaoTree.c
	$(CC) $(CFLAGS) -c BaoTree.c

BaoError.o: BaoError.h BaoError.c
	$(CC) $(CFLAGS) -c BaoError.c

BaoTreeTest: $(OBJ) BaoTreeTest.c
	$(CC) $(CFLAGS) -o BaoTreeTest $^

tests: $(TESTS)

clean:
	rm -vf *.o $(TESTS)
