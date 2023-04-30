
all:
	gcc -c lista.c
	ar rcs liblista.a lista.o
	gcc -o wworker wworker.c -pthread 
	gcc -o sserver sserver.c -pthread -L. -llista
	gcc -o clientee clientee.c -pthread
	
