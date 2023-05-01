#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <time.h> 
#include <pthread.h>
#include <signal.h>

#define BUFFER_SIZE 1024

// Include this definition at the top of your file, after the includes

typedef enum {
    IDLE,
    WORKING
} worker_status;

typedef struct client_data{
    int socket;
    struct sockaddr_in* client_addr;
    worker_status status;
} client_data;

typedef struct Node {
    client_data* data;
    struct Node* next;
} Node;

Node* worker_list = NULL;

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

Node* create_list() {
    return NULL;
}

void add_elem(Node** head, client_data* data) {
    Node* new_elem = (Node*) malloc(sizeof(Node));
    new_elem->data = data;
    new_elem->data->status = IDLE;
    new_elem->next = *head;
    *head = new_elem;
}

void remove_elem(Node** head, client_data* data) {
    Node* current = *head;
    Node* previous = NULL;
    
    while (current != NULL) {
        if (current->data == data) {
            if (previous == NULL) {
                *head = current->next;
            } else {
                previous->next = current->next;
            }
            free(current);
            return;
        }
        previous = current;
        current = current->next;
    }
}

void display_list(Node* head) {
    Node* current = head;
    printf("Available workers:\n");
    while (current != NULL) {
        printf("Worker %s:%d (%d)\n", inet_ntoa(current->data->client_addr->sin_addr), ntohs(current->data->client_addr->sin_port), current->data->status);
        current = current->next;
    }
    fflush(stdout);
}

Node* find_idle_worker(Node* head){
    if (head == NULL) {
        return NULL;
    }
    Node* current = head;
    while (current != NULL) {
        if (current->data->status == IDLE){
            return current;
        } else {
            current = current->next;
        } 
    }
    return NULL;
}

void exit_handler(int sig) {
    printf("\nServer disconnected\n");
    fflush(stdout);
    exit(1);
}

int recv_msg(int socket, char* buffer){
    int i = 0, bytes_received;

    while((bytes_received = recv(socket, &buffer[i], 1, 0)) > 0){
        if(buffer[i] == '\0'){
            break;
        }
        i++;
    }

    if(bytes_received < 0){
        return bytes_received;
    }
    return i;
}

void * client_handle(void* client_data){
    struct client_data *client = (struct client_data *)client_data;
    char buffer[BUFFER_SIZE];
    memset(buffer, 0, BUFFER_SIZE);

    printf("Connection received from %s:%d\n", inet_ntoa(client->client_addr->sin_addr), ntohs(client->client_addr->sin_port));
    fflush(stdout);

    if (recv_msg(client->socket, buffer) < 0) {
        perror("Error receiving request");
        exit(EXIT_FAILURE);
    }

    if (strcmp(buffer, "worker") == 0) {
        pthread_mutex_lock(&mutex);
        add_elem(&worker_list, client);
        pthread_mutex_unlock(&mutex);

        printf("Worker %s:%d added\n", inet_ntoa(client->client_addr->sin_addr), ntohs(client->client_addr->sin_port));
        display_list(worker_list);
    }

    else if (strcmp(buffer, "client") == 0){

        Node* idle_worker = find_idle_worker(worker_list);

        if(idle_worker == NULL){
            printf("No available workers!\n");
            snprintf(buffer, sizeof(buffer), "%s", "System is busy.");  
            send(client->socket, buffer, strlen(buffer) + 1, 0);
        } else {

            if (recv_msg(client->socket, buffer) < 0) {
                perror("Error receiving request");
                exit(EXIT_FAILURE);
            }

            idle_worker->data->status = WORKING;
            send(idle_worker->data->socket, buffer, strlen(buffer) + 1, 0);

            printf("Worker %s:%d (%d) serving client %s:%d\n", inet_ntoa(idle_worker->data->client_addr->sin_addr), ntohs(idle_worker->data->client_addr->sin_port), idle_worker->data->status, inet_ntoa(client->client_addr->sin_addr), ntohs(client->client_addr->sin_port));

            if (recv_msg(idle_worker->data->socket, buffer) < 0) {
                perror("Error receiving request");
                exit(EXIT_FAILURE);
            }

            idle_worker->data->status = IDLE;
            printf("Worker response: %s\n", buffer);
            send(client->socket, buffer, strlen(buffer) + 1, 0);

            close(client->socket);
            free(client->client_addr);
            free(client);
        }
    } else {
        printf("Invalid identification: %s\n", buffer);
    }
    return NULL;
}

int main(int argc, char *argv[]){
    signal(SIGINT, exit_handler);

    int server_socket = 0; 
    struct sockaddr_in server_addr; 
    int addr_len;
    struct client_data *client;
    pthread_t thread;

    worker_list = create_list();

    if ((server_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket");
        return 1;
    }

    memset(&server_addr, 0, sizeof(server_addr));

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(5000); 

    if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind");
        return 1;
    } 

    listen(server_socket, 10); 

    while(1) {

        client = (struct client_data *)malloc(sizeof(struct client_data));
        client->client_addr = (struct sockaddr_in*)malloc(sizeof(struct sockaddr_in));
        addr_len = sizeof(struct sockaddr_in);

        client->socket = accept(server_socket, (struct sockaddr*)client->client_addr, (socklen_t*)&addr_len); 
        pthread_create(&thread, NULL, client_handle, (void *)client);
        pthread_detach(thread);
    }
}
