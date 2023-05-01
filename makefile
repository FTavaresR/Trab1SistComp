
all:
	gcc -o sserver sserver.c -pthread
	gcc -o wworker wworker.c -pthread 
	gcc -o clientee clientee.c -pthread
	
