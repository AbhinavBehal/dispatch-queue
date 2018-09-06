# Compiles all of the tests.
all: num_cores test1 test2 test3 test4 test5 testFor

num_cores: num_cores.c
	gcc num_cores.c -o num_cores

test1: test1.c dispatchQueue.o
	gcc test1.c dispatchQueue.o -o test1 -pthread

test2: test2.c dispatchQueue.o
	gcc test2.c dispatchQueue.o -o test2 -pthread

test3: test3.c dispatchQueue.o
	gcc test3.c dispatchQueue.o -o test3 -pthread

test4: test4.c dispatchQueue.o
	gcc test4.c dispatchQueue.o -o test4 -pthread

test5: test5.c dispatchQueue.o
	gcc test5.c dispatchQueue.o -o test5 -pthread

testFor: testFor.c dispatchQueue.o
	gcc testFor.c dispatchQueue.o -o testFor -pthread

dispatchQueue.o: dispatchQueue.c dispatchQueue.h
	gcc -c dispatchQueue.c
