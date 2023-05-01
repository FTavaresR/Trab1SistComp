#include <arpa/inet.h>

#include <netinet/in.h>

#include <stdio.h>

#include <stdlib.h>

#include <string.h>

#include <sys/socket.h>

#include <unistd.h>

#include <netinet/tcp.h>



#define BUFFER_SIZE 1024

#define PORT 5000



// A função lê um byte de cada vez do socket sk e armazena no array b até que encontre um byte nulo 



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



// responsável pelas operações de soma, subtração, multiplicação e divisão



double perform_operation(const char *operation, double a, double b) {

     sleep(2);

    if (strcmp(operation, "add") == 0) {

        return a + b;

    } else if (strcmp(operation, "subtract") == 0) {

        return a - b;

    } else if (strcmp(operation, "multiply") == 0) {

        return a * b;

    } else if (strcmp(operation, "divide") == 0) {

        return a / b;

    } else {

        return 0.0;

    }

}



int main(int argc, char *argv[]) {

    int sockfd;

    struct sockaddr_in server_addr;

    const char worker_hello[] = "worker";

    

    // verifica se o número de argumentos passados na linha de comando é menor que 2, o que significa que o endereço IP do servidor não foi informado



    if (argc < 2){

        printf("Usage: %s <ip address>\n", argv[0]);

        exit(0);

    }



    // cria um novo socket usando a função socket() 

    sockfd = socket(AF_INET, SOCK_STREAM, TCP_NODELAY && !TCP_CORK);

    if (sockfd < 0) {

        perror("Error creating socket");

        exit(EXIT_FAILURE);

    }



    /* Configura o endereço do server */

    memset(&server_addr, 0, sizeof(server_addr));

    server_addr.sin_family = AF_INET;

    server_addr.sin_port = htons(PORT);

    server_addr.sin_addr.s_addr = inet_addr(argv[1]);



    /* Conecta ao server */

    if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {

        perror("Error connecting to server");

        exit(EXIT_FAILURE);

    }



    /* Envia uma mensagem identificando a si mesmo como worker */

    if (send(sockfd, worker_hello, strlen(worker_hello) + 1, 0) < 0) {

        perror("Error sending worker hello");

        exit(EXIT_FAILURE);

    }



    char buffer[BUFFER_SIZE];

    while (1) {

        /* Recebe a requisição */

        memset(buffer, 0, BUFFER_SIZE);

        if (recv_msg(sockfd, buffer) < 0) {

            perror("Error receiving request");

            exit(EXIT_FAILURE);

        }



        /* Se o server envia "quit", fecha a conexão e sai */

        if (strcmp(buffer, "quit") == 0) {

            printf("Worker quitting...\n");

            break;

        }



        /* Analisa a solicitação e execute a operação */

        char operation[32];

        double a, b;

        // printf("buffer: %s",buffer);

        sscanf(buffer, "%s %lf %lf", operation, &a, &b);

        printf("Worker received request: %s %lf %lf\n", operation, a, b);

        fflush(stdout);

        double result = perform_operation(operation, a, b);



        /* Envia o resultado de volta ao server */

        snprintf(buffer, BUFFER_SIZE, "%.2lf", result);

        printf("Result: %s\n",buffer);

        fflush(stdout);



        if (send(sockfd, buffer, strlen(buffer) + 1, 0) < 0) {

            perror("Error sending result");

            exit(EXIT_FAILURE);

        }

        printf("Mensagem enviada com sucesso!\n");

    }



    /* Fecha o socket */

    close(sockfd);



    return 0;

}
