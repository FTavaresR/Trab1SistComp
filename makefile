
all:
	gcc -o worker worker.c 
	gcc -o client client.c
	gcc -o server server.c -pthread
