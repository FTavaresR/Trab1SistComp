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
#include "lista.h"

#define BUFFER_SIZE 1024

No* lista = NULL;

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

No* criar_lista() {
    return NULL;
}

void add_elem(No** head, client_data* data) {
    No* novel = (No*) malloc(sizeof(No));
    novel->data = data;
    novel->data->estado=OCIOSO;
    novel->proximo = *head;
    *head = novel;
}

void exc_elem(No** head, client_data* data) {
    No* atual = *head;
    No* anterior = NULL;
    
    while (atual != NULL) {
        if (atual->data == data) {
            if (anterior == NULL) {
                *head = atual->proximo;
            } else {
                anterior->proximo = atual->proximo;
            }
            free(atual);
            return;
        }
        anterior = atual;
        atual = atual->proximo;
    }
}

void exibir(No* head) {
    No* atual = head;
    printf("Workers disponíveis:\n");
    while (atual != NULL) {
        printf("Worker %s:%d (%d)\n", inet_ntoa(atual->data->client_addr->sin_addr), ntohs(atual->data->client_addr->sin_port),atual->data->estado);
        atual = atual->proximo;
    }
    fflush(stdout);
}

No* busca_worker(No* head){
    if (head == NULL) {
        return NULL;
    }
    No* atual = head;
    while (atual != NULL) {
        if (atual->data->estado == OCIOSO){
            return atual;
        } else {
            atual = atual->proximo;
        } 
    }
    return NULL;
}


void exit_handler(int);

void exit_handler(int sig) {
    printf("\n O server foi desconectado \n");
    fflush(stdout);
    exit(1);
}

int recv_msg(int sk, char* b){

    int i = 0, bac;

    while((bac = recv(sk,&b[i],1,0))>0){
        if(b[i] == '\0'){
            break;
        }
        i++;
    }

    if(bac < 0){
        return bac;
    }
    return i;
}

void * client_handle(void* dc){
    struct client_data *client = (struct client_data *)dc;
    char buff[BUFFER_SIZE];
    memset(buff, 0, BUFFER_SIZE);

    printf("Conexão recebida de %s:%d\n", inet_ntoa(client->client_addr->sin_addr), ntohs(client->client_addr->sin_port));
    fflush(stdout);

   if (recv_msg(client->sk, buff) < 0) {
        perror("Erro ao receber solicitacao");
        exit(EXIT_FAILURE);
        }

    if (strcmp(buff, "worker") == 0) {
        pthread_mutex_lock(&mutex);
            add_elem(&lista, client);
        pthread_mutex_unlock(&mutex);

        printf("O worker %s:%d foi adicionado\n", inet_ntoa(client->client_addr->sin_addr), ntohs(client->client_addr->sin_port));
        exibir(lista);
    }

    else if (strcmp(buff, "client") == 0){

        No* myWorker = busca_worker(lista);

        if(myWorker==NULL){
            printf("Sem workers disponíveis!\n");
            snprintf(buff, sizeof(buff), "%s","Sistema está ocupado.");  
            send(client->sk, buff, strlen(buff)+1, 0);
        }else{

            if (recv_msg(client->sk, buff) < 0) {
                perror("Erro ao receber solicitacao");
                exit(EXIT_FAILURE);
            }

            myWorker->data->estado=TRABALHANDO;
            send(myWorker->data->sk, buff, strlen(buff)+1, 0);

            printf("O worker %s:%d (%d) está a servico do cliente %s:%d\n", inet_ntoa(myWorker->data->client_addr->sin_addr), ntohs(myWorker->data->client_addr->sin_port),myWorker->data->estado,inet_ntoa(client->client_addr->sin_addr), ntohs(client->client_addr->sin_port));

            if (recv_msg(myWorker->data->sk, buff) < 0) {
                perror("Erro ao receber solicitacao");
                exit(EXIT_FAILURE);
            }

            myWorker->data->estado=OCIOSO;
            printf("Resposta do worker: %s\n",buff);
            send(client->sk, buff, strlen(buff)+1, 0);

            close(client->sk);
            free(client->client_addr);
            free(client);
        }
    }else{
        printf("Identificação inválida: %s\n",buff);
    }
    return NULL;
}

int main(int argc, char *argv[]){
    signal(SIGINT, exit_handler);

    int lfd = 0; 
    struct sockaddr_in ende_serv; 
    int tam_ende;
    struct client_data *dc;
    pthread_t thr;

    lista = criar_lista();

    if ( (lfd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
    	perror("socket");
    	return 1;
    }

    memset(&ende_serv, 0, sizeof(ende_serv));

    ende_serv.sin_family = AF_INET;
    ende_serv.sin_addr.s_addr = htonl(INADDR_ANY);
    ende_serv.sin_port = htons(5000); 

    if (bind(lfd, (struct sockaddr*)&ende_serv, sizeof(ende_serv)) < 0){
    	perror("bind");
    	return 1;
    } 

    listen(lfd, 10); 

    while(1) {

        dc = (struct client_data *)malloc(sizeof(struct client_data));
        dc->client_addr = (struct sockaddr_in*)malloc(sizeof(struct sockaddr_in));
        tam_ende = sizeof(struct sockaddr_in);

        dc->sk = accept(lfd, (struct sockaddr*)dc->client_addr, (socklen_t*)&tam_ende); 
        pthread_create(&thr, NULL, client_handle, (void *)dc);
        pthread_detach(thr);
     }
}