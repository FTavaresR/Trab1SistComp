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



// usada para identificar se o worker est� ocioso ou ocupado



typedef enum {

    IDLE,

    WORKING

} worker_status; 





// Usada para armazenar informa��es sobre o cliente, incluindo o seu socket e o seu endere�o



typedef struct client_data{

    int socket;

    struct sockaddr_in* client_addr;

    worker_status status;

} client_data;



// � usada para criar uma lista encadeada, onde cada n� (Node) cont�m um ponteiro para client_data e um ponteiro para o pr�ximo n� na lista



typedef struct Node {

    client_data* data;

    struct Node* next;

} Node;



Node* worker_list = NULL; // vari�vel global inicializada como NULL -> cria uma lista vazia de trabalhadores



pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER; // cria um mutex pronto para uso (inicializa��o do mutex)



// cria uma nova lista encadeada vazia e retorna um ponteiro para o n� inicial da lista (NULL)



Node* create_list() {

    return NULL;

}



// adiciona um novo elemento (node) no in�cio da lista

// A fun��o aloca mem�ria para um novo node, inicializa seus campos de dados com as informa��es do cliente e insere o novo node no in�cio da lista

void add_elem(Node** head, client_data* data) {

    Node* new_elem = (Node*) malloc(sizeof(Node));

    new_elem->data = data;

    new_elem->data->status = IDLE;

    new_elem->next = *head;

    *head = new_elem;

}



//  A fun��o percorre a lista buscando pelo node que cont�m as informa��es do cliente e remove o node da lista. Se o node estiver no in�cio da lista, o ponteiro para o in�cio da lista � atualizado para o pr�ximo node. A fun��o libera a mem�ria alocada para o node removido



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



// Imprime na sa�da padr�o a lista de workers dispon�veis 

// As informa��es impressas incluem o endere�o IP e porta do cliente e o status do worker (dispon�vel ou ocupado)



void display_list(Node* head) {

    Node* current = head;

    printf("Workers dispon�veis:\n");

    while (current != NULL) {

        printf("Worker %s:%d (%d)\n", inet_ntoa(current->data->client_addr->sin_addr), ntohs(current->data->client_addr->sin_port), current->data->status);

        current = current->next;

    }

    fflush(stdout);

}



//  procura por um trabalhador (worker) na lista encadeada passada como par�metro que esteja com status IDLE (ocioso), retornando o n� da lista correspondente ao trabalhador encontrado. Caso n�o encontre um trabalhador ocioso, a fun��o retorna NULL.



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





void exit_handler(int sig) { // � chamado quando o usu�rio pressiona ctrl+c

    printf("\n O server foi desconectado \n");

    fflush(stdout);

    exit(1);

}



// Recebe uma mensagem de um socket e armazena em um buffer.



int recv_msg(int socket, char* buffer){ // Par�metros: socket de onde a vari�vel ser� recebida e buffer de caracteres onde a mensagem ser� armazenada

    int i = 0, bytes_received;



    while((bytes_received = recv(socket, &buffer[i], 1, 0)) > 0){

        if(buffer[i] == '\0'){

            break;

        }

        i++;

    }



    if(bytes_received < 0){ // condi��o de um erro na recep��o da mensagem

        return bytes_received; 

    }

    return i; // indica que a fun��o recv_msg leu com sucesso i bytes do socket e armazenou-os no buffer

}



// Implementa a fun��o de tratamento de clientes em um servidor. A fun��o � executada em uma thread separada para cada cliente conectado



void * client_handle(void* client_data){

    struct client_data *client = (struct client_data *)client_data;

    char buffer[BUFFER_SIZE];

    memset(buffer, 0, BUFFER_SIZE); // preenche o bloco de mem�ria de buffer com zeros



    printf("Conex�o recebida de %s:%d\n", inet_ntoa(client->client_addr->sin_addr), ntohs(client->client_addr->sin_port));

// inet_ntoa: converte um endere�o IP em formato bin�rio para uma string no formato decimal ponto

    fflush(stdout);



    if (recv_msg(client->socket, buffer) < 0) { // indicativo de erro

        perror("Erro ao receber solicitacao");

        exit(EXIT_FAILURE);

    }



// Se a mensagem for "worker", a fun��o adiciona o cliente � lista de trabalhadores usando a fun��o add_elem

    if (strcmp(buffer, "worker") == 0) {

        pthread_mutex_lock(&mutex); // utilizado para evitar condi��o de corrida

        add_elem(&worker_list, client);

        pthread_mutex_unlock(&mutex);



        printf("O worker %s:%d foi adicionado\n", inet_ntoa(client->client_addr->sin_addr), ntohs(client->client_addr->sin_port));

        display_list(worker_list);

    }



// Se a mensagem for "client", a fun��o procura um trabalhador dispon�vel na lista de trabalhadores usando a fun��o find_idle_worker



    else if (strcmp(buffer, "client") == 0){



        Node* idle_worker = find_idle_worker(worker_list);



        if(idle_worker == NULL){ // Caso n�o tenha workers dispon�veis

            printf("Sem workers dispon�veis!\n");

            snprintf(buffer, sizeof(buffer), "%s", "Sistema esta ocupado.");  

            send(client->socket, buffer, strlen(buffer) + 1, 0);

        } else { // se a mensagem n�o for worker nem client



            if (recv_msg(client->socket, buffer) < 0) { 

                perror("Erro ao receber solicitacao");

                exit(EXIT_FAILURE);

            }



            idle_worker->data->status = WORKING; // worker que foi escolhido para atender o cliente 

            send(idle_worker->data->socket, buffer, strlen(buffer) + 1, 0);



// nthos: converte a ordem dos bytes dos dados recebidos para o formato correto utilizado pelo sistema operacional em quest�o

            printf("O worker %s:%d (%d) est� a servico do cliente %s:%d\n", inet_ntoa(idle_worker->data->client_addr->sin_addr), ntohs(idle_worker->data->client_addr->sin_port), idle_worker->data->status, inet_ntoa(client->client_addr->sin_addr), ntohs(client->client_addr->sin_port));



            if (recv_msg(idle_worker->data->socket, buffer) < 0) {

                perror("Erro ao receber solicitacao");

                exit(EXIT_FAILURE);

            }



            idle_worker->data->status = IDLE;

            printf("Resposta do worker: %s\n", buffer);

            send(client->socket, buffer, strlen(buffer) + 1, 0);



            close(client->socket);

            free(client->client_addr);

            free(client);

        }

    } else {

        printf("Identifica��o inv�lida: %s\n", buffer);

    }

    return NULL;

}



// Fun��o main inicializa o servidor e fica em loop infinito aguardando novas conex�es

// Quando uma nova conex�o � estabelecida, a fun��o client_handle � chamada em uma nova thread para tratar a requisi��o do cliente



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



    memset(&server_addr, 0, sizeof(server_addr)); // preenche o bloco de mem�ria de server_addr com zeros -> Inicializar um array com todos os elementos iguais a 0



    server_addr.sin_family = AF_INET; // especifica que o endere�o do servidor usa a fam�lia de protocolos IPv4

    server_addr.sin_addr.s_addr = htonl(INADDR_ANY); // A fun��o htonl � usada para converter o n�mero inteiro longo INADDR_ANY no formato de rede

    server_addr.sin_port = htons(5000); // A fun��o htons � usada para converter o n�mero de porta do servidor 5000 no formato de rede



// bind: respons�vel por associar o endere�o e a porta do servidor com o socket criado

//  Se a fun��o retornar um valor menor que zero, significa que ocorreu um erro na associa��o

    if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) { 

        perror("bind");

        return 1; // indica que a inicializa��o falhou

    } 



    listen(server_socket, 10); // coloca o socket em fun��o de escuta, permitindo que o servidor aceite conex�o de clientes

    // tamanho m�ximo da fila de conex�es = 10



// loop while: fica aguardando infinitamente pela conex�o de clientes



    while(1) {



        client = (struct client_data *)malloc(sizeof(struct client_data));

        client->client_addr = (struct sockaddr_in*)malloc(sizeof(struct sockaddr_in));

        addr_len = sizeof(struct sockaddr_in);



        client->socket = accept(server_socket, (struct sockaddr*)client->client_addr, (socklen_t*)&addr_len); 

 // fun��o accept: chamada para aceitar uma nova conex�o de cliente -> Essa fun��o bloqueia a execu��o do programa at� que um cliente se conecte

        pthread_create(&thread, NULL, client_handle, (void *)client);

// � usada para criar uma nova thread para tratar a requisi��o desse cliente, evitando assim que a thread principal do servidor seja bloqueada por um longo per�odo

        pthread_detach(thread);

// � chamada para indicar que a thread pode ser destru�da assim que terminar a execu��o da fun��o client_handle(). 

    }

}
