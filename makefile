
all:
	gcc -o wworker wworker.c 
	gcc -o clientee clientee.c
	gcc -o server_t server_t.c -pthread
