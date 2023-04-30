/* Adaptado de https://www.thegeekstuff.com/2011/12/c-socket-programming/?utm_source=feedburner */

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
//TODO: implementar soluções para evitar condição de corrida na lista!
No* lista = NULL;
pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER; //mutex para controle de escrita na lista

void exit_handler(int);

void exit_handler(int sig) {
    printf("\nSever desligando...\n");
    fflush(stdout);
    exit(1);
}

//função de leitura segura:
int recv_msg(int sk, char* b){
    int i = 0, ret;
    while((ret = recv(sk,&b[i],1,0))>0){
        if(b[i] == '\0'){
            break;
        }
        i++;
    }
    if(ret < 0){
        return ret;
    }
    return i;
}

void * client_handle(void* cd){
    struct client_data *client = (struct client_data *)cd;
    char buffer[BUFFER_SIZE];
    memset(buffer, 0, BUFFER_SIZE);

    /* Imprime IP e porta do cliente. */
    printf("Received connection from %s:%d\n", inet_ntoa(client->client_addr->sin_addr), ntohs(client->client_addr->sin_port));
    fflush(stdout);

   //Recebe a mensagem do client/worker
   if (recv_msg(client->sk, buffer) < 0) {
        perror("Error receiving request");
        exit(EXIT_FAILURE);
        }
    //se for um work,adiciona-o a lista
    if (strcmp(buffer, "worker") == 0) {
        pthread_mutex_lock(&m);
           adicionar_elemento(&lista,client);
        pthread_mutex_unlock(&m);

        printf("Worker %s:%d adicionado a lista!\n", inet_ntoa(client->client_addr->sin_addr), ntohs(client->client_addr->sin_port));
        print_lista(lista);
    }

    //se for um cliente:
    else if (strcmp(buffer, "client") == 0){
        No* myWorker = busca_worker(lista);
        if(myWorker==NULL){
            printf("Não há workers disponíveis!\n");
            snprintf(buffer, sizeof(buffer), "%s","SISTEMA OCUPADO! Tente mais tarde.");  
            send(client->sk, buffer, strlen(buffer)+1, 0);
        }else{
            //recebe a solicitação do client
            if (recv_msg(client->sk, buffer) < 0) {
                perror("Error receiving request");
                exit(EXIT_FAILURE);
            }
            //coloca o worker para porcessar a solicitação
            myWorker->data->estado=TRABALHANDO;
            send(myWorker->data->sk, buffer, strlen(buffer)+1, 0);
            printf("Worker %s:%d (%d) trabalhando para o cliente %s:%d\n", inet_ntoa(myWorker->data->client_addr->sin_addr), ntohs(myWorker->data->client_addr->sin_port),myWorker->data->estado,inet_ntoa(client->client_addr->sin_addr), ntohs(client->client_addr->sin_port));   

             //aguarda a resposta do worker e libera-o para outro trabalho:
            if (recv_msg(myWorker->data->sk, buffer) < 0) {
                perror("Error receiving request");
                exit(EXIT_FAILURE);
            }
            myWorker->data->estado=OCIOSO;
            printf("resposta do worker: %s\n",buffer);
            send(client->sk, buffer, strlen(buffer)+1, 0);

            /* Fecha conexão com o cliente. */
            close(client->sk);
            free(client->client_addr);
            free(client);
        }
    }else{
        printf("Identificação inválida: %s\n",buffer);
    }
    //print_lista(lista);
    //fflush(stdout);
    return NULL;
}

int main(int argc, char *argv[])
{
    signal(SIGINT, exit_handler);

    int listenfd = 0;
    struct sockaddr_in serv_addr; 
    int addrlen;
    struct client_data *cd;
    pthread_t thr;
    lista = criar_lista();
    
    /* Cria o Socket: SOCK_STREAM = TCP */
    if ( (listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
    	perror("socket");
    	return 1;
    }

    memset(&serv_addr, 0, sizeof(serv_addr));

	/* Configura servidor para receber conexoes de qualquer endereço:
	 * INADDR_ANY e ouvir na porta 5000 */ 
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(5000); 

	/* Associa o socket a estrutura sockaddr_in */
    if (bind(listenfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0){
    	perror("bind");
    	return 1;
    } 

	/* Inicia a escuta na porta */
    listen(listenfd, 10); 

    while(1) {
        cd = (struct client_data *)malloc(sizeof(struct client_data));
        cd->client_addr = (struct sockaddr_in*)malloc(sizeof(struct sockaddr_in));
        addrlen = sizeof(struct sockaddr_in);

		/* Aguarda a conexão */	
        cd->sk = accept(listenfd, (struct sockaddr*)cd->client_addr, (socklen_t*)&addrlen); 

        pthread_create(&thr, NULL, client_handle, (void *)cd);
        pthread_detach(thr);

     }
}