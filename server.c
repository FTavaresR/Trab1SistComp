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



// usada para identificar se o worker está ocioso ou ocupado



typedef enum {

    IDLE,

    WORKING

} worker_status; 





// Usada para armazenar informações sobre o cliente, incluindo o seu socket e o seu endereço



typedef struct client_data{

    int socket;

    struct sockaddr_in* client_addr;

    worker_status status;

} client_data;



// É usada para criar uma lista encadeada, onde cada nó (Node) contém um ponteiro para client_data e um ponteiro para o próximo nó na lista



typedef struct Node {

    client_data* data;

    struct Node* next;

} Node;



Node* worker_list = NULL; // variável global inicializada como NULL -> cria uma lista vazia de trabalhadores



pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER; // cria um mutex pronto para uso (inicialização do mutex)



// cria uma nova lista encadeada vazia e retorna um ponteiro para o nó inicial da lista (NULL)



Node* create_list() {

    return NULL;

}



// adiciona um novo elemento (node) no início da lista

// A função aloca memória para um novo node, inicializa seus campos de dados com as informações do cliente e insere o novo node no início da lista

void add_elem(Node** head, client_data* data) {

    Node* new_elem = (Node*) malloc(sizeof(Node));

    new_elem->data = data;

    new_elem->data->status = IDLE;

    new_elem->next = *head;

    *head = new_elem;

}



//  A função percorre a lista buscando pelo node que contém as informações do cliente e remove o node da lista. Se o node estiver no início da lista, o ponteiro para o início da lista é atualizado para o próximo node. A função libera a memória alocada para o node removido



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



// Imprime na saída padrão a lista de workers disponíveis 

// As informações impressas incluem o endereço IP e porta do cliente e o status do worker (disponível ou ocupado)



void display_list(Node* head) {

    Node* current = head;

    printf("Workers disponíveis:\n");

    while (current != NULL) {

        printf("Worker %s:%d (%d)\n", inet_ntoa(current->data->client_addr->sin_addr), ntohs(current->data->client_addr->sin_port), current->data->status);

        current = current->next;

    }

    fflush(stdout);

}



//  procura por um trabalhador (worker) na lista encadeada passada como parâmetro que esteja com status IDLE (ocioso), retornando o nó da lista correspondente ao trabalhador encontrado. Caso não encontre um trabalhador ocioso, a função retorna NULL.



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





void exit_handler(int sig) { // É chamado quando o usuário pressiona ctrl+c

    printf("\n O server foi desconectado \n");

    fflush(stdout);

    exit(1);

}



// Recebe uma mensagem de um socket e armazena em um buffer.



int recv_msg(int socket, char* buffer){ // Parâmetros: socket de onde a variável será recebida e buffer de caracteres onde a mensagem será armazenada

    int i = 0, bytes_received;



    while((bytes_received = recv(socket, &buffer[i], 1, 0)) > 0){

        if(buffer[i] == '\0'){

            break;

        }

        i++;

    }



    if(bytes_received < 0){ // condição de um erro na recepção da mensagem

        return bytes_received; 

    }

    return i; // indica que a função recv_msg leu com sucesso i bytes do socket e armazenou-os no buffer

}



// Implementa a função de tratamento de clientes em um servidor. A função é executada em uma thread separada para cada cliente conectado



void * client_handle(void* client_data){

    struct client_data *client = (struct client_data *)client_data;

    char buffer[BUFFER_SIZE];

    memset(buffer, 0, BUFFER_SIZE); // preenche o bloco de memória de buffer com zeros



    printf("Conexão recebida de %s:%d\n", inet_ntoa(client->client_addr->sin_addr), ntohs(client->client_addr->sin_port));

// inet_ntoa: converte um endereço IP em formato binário para uma string no formato decimal ponto

    fflush(stdout);



    if (recv_msg(client->socket, buffer) < 0) { // indicativo de erro

        perror("Erro ao receber solicitacao");

        exit(EXIT_FAILURE);

    }



// Se a mensagem for "worker", a função adiciona o cliente à lista de trabalhadores usando a função add_elem

    if (strcmp(buffer, "worker") == 0) {

        pthread_mutex_lock(&mutex); // utilizado para evitar condição de corrida

        add_elem(&worker_list, client);

        pthread_mutex_unlock(&mutex);



        printf("O worker %s:%d foi adicionado\n", inet_ntoa(client->client_addr->sin_addr), ntohs(client->client_addr->sin_port));

        display_list(worker_list);

    }



// Se a mensagem for "client", a função procura um trabalhador disponível na lista de trabalhadores usando a função find_idle_worker



    else if (strcmp(buffer, "client") == 0){



        Node* idle_worker = find_idle_worker(worker_list);



        if(idle_worker == NULL){ // Caso não tenha workers disponíveis

            printf("Sem workers disponíveis!\n");

            snprintf(buffer, sizeof(buffer), "%s", "Sistema esta ocupado.");  

            send(client->socket, buffer, strlen(buffer) + 1, 0);

        } else { // se a mensagem não for worker nem client



            if (recv_msg(client->socket, buffer) < 0) { 

                perror("Erro ao receber solicitacao");

                exit(EXIT_FAILURE);

            }



            idle_worker->data->status = WORKING; // worker que foi escolhido para atender o cliente 

            send(idle_worker->data->socket, buffer, strlen(buffer) + 1, 0);



// nthos: converte a ordem dos bytes dos dados recebidos para o formato correto utilizado pelo sistema operacional em questão

            printf("O worker %s:%d (%d) está a servico do cliente %s:%d\n", inet_ntoa(idle_worker->data->client_addr->sin_addr), ntohs(idle_worker->data->client_addr->sin_port), idle_worker->data->status, inet_ntoa(client->client_addr->sin_addr), ntohs(client->client_addr->sin_port));



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

        printf("Identificação inválida: %s\n", buffer);

    }

    return NULL;

}



// Função main inicializa o servidor e fica em loop infinito aguardando novas conexões

// Quando uma nova conexão é estabelecida, a função client_handle é chamada em uma nova thread para tratar a requisição do cliente



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



    memset(&server_addr, 0, sizeof(server_addr)); // preenche o bloco de memória de server_addr com zeros -> Inicializar um array com todos os elementos iguais a 0



    server_addr.sin_family = AF_INET; // especifica que o endereço do servidor usa a família de protocolos IPv4

    server_addr.sin_addr.s_addr = htonl(INADDR_ANY); // A função htonl é usada para converter o número inteiro longo INADDR_ANY no formato de rede

    server_addr.sin_port = htons(5000); // A função htons é usada para converter o número de porta do servidor 5000 no formato de rede



// bind: responsável por associar o endereço e a porta do servidor com o socket criado

//  Se a função retornar um valor menor que zero, significa que ocorreu um erro na associação

    if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) { 

        perror("bind");

        return 1; // indica que a inicialização falhou

    } 



    listen(server_socket, 10); // coloca o socket em função de escuta, permitindo que o servidor aceite conexão de clientes

    // tamanho máximo da fila de conexões = 10



// loop while: fica aguardando infinitamente pela conexão de clientes



    while(1) {



        client = (struct client_data *)malloc(sizeof(struct client_data));

        client->client_addr = (struct sockaddr_in*)malloc(sizeof(struct sockaddr_in));

        addr_len = sizeof(struct sockaddr_in);



        client->socket = accept(server_socket, (struct sockaddr*)client->client_addr, (socklen_t*)&addr_len); 

 // função accept: chamada para aceitar uma nova conexão de cliente -> Essa função bloqueia a execução do programa até que um cliente se conecte

        pthread_create(&thread, NULL, client_handle, (void *)client);

// É usada para criar uma nova thread para tratar a requisição desse cliente, evitando assim que a thread principal do servidor seja bloqueada por um longo período

        pthread_detach(thread);

// É chamada para indicar que a thread pode ser destruída assim que terminar a execução da função client_handle(). 

    }

}
