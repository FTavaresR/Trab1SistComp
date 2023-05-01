#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h> 
#include <time.h>
#include <unistd.h>

// Utilizada para receber uma mensagem em um socket, caracter a caracter, até que seja encontrada a marcação de fim de mensagem (caractere nulo)

int recv_msg(int sk, char* b){
    int i = 0, ret;
    //lendo os caracteres do socket um por um, utilizando a função recv da biblioteca sys/socket.h
    while((ret = recv(sk,&b[i],1,0))>0){
        if(b[i] == '\0'){ // indica que a mensagem terminou
            break;
        }
        i++;
    }

    if(ret < 0){ // indicativo de erro
        return ret;
    }

    return i; // Quando a função termina de ler a mensagem, ela retorna o número de caracteres lidos (sem contar com o caractere nulo), que é armazenado em i
}

int main(int argc, char *argv[])
{

// inicialização de variáveis
    int sockfd = 0, n = 0;
    char recvBuff[1024];
    struct sockaddr_in serv_addr;  // struct definida na biblioteca netinet/in.h -> usada para as informações de endereço do servidor a ser conectado -> IP e a porta a ser usada na conexão
    const char client_hello[] = "client"; // 
    char client_msg[20]; // usada para armazenar a mensagem que o usuário digitar para ser enviada para o servidor

    // Checa o número de argumentos necessários: execução, ip, operação, número 1, número 2
    if(argc != 5) 
    {
        printf("\n Uso: %s <ip of server> <operation> <number1> <number2> \n",argv[0]);
        return 1;
    } 

    // Determina as strings referentes a cada parâmetro: operação, número 1 e número 2
    const char* operation = argv[2]; 
    int number1 = atoi(argv[3]);
    int number2 = atoi(argv[4]);

    // 
    snprintf(client_msg, sizeof(client_msg), "%s %d %d", operation, number1, number2); //  formatar a mensagem com base nas informações que o usuário passou como argumentos na linha de comando (operação, número 1 e número 2) e armazena essa mensagem na variável client_msg

    memset(recvBuff, 0,sizeof(recvBuff)); 
    
    if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("socket");
        return 1;
    } 

    memset(&serv_addr, 0, sizeof(serv_addr)); // zera a estrutura de dados serv_addr
    serv_addr.sin_family = AF_INET; // indica o uso do IPv4
    serv_addr.sin_port = htons(5000); // define o número da porta para o socket

// converte o endereço IP passado como argumento argv[1] para o formato necessário para a conexão
    if(inet_pton(AF_INET, argv[1], &serv_addr.sin_addr)<=0)
    {
        perror("inet_pton");
        return 1;
    } 

// A função connect é usada para estabelecer a conexão com o servidor -> caso falhe: perror
    if( connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
    	perror("connect");
       	return 1;
    } 

// envia uma mensagem para o servidor através do socket -> caso a mensagem não possa ser enviada -> perror
    if (send(sockfd, client_hello, strlen(client_hello) + 1, 0) < 0) {
        perror("Error sending client message");
        exit(EXIT_FAILURE);
    }

    printf("Enviando %s \n",client_msg); // imprime a mensagem a ser enviada para o servidor
    fflush(stdout); // limpa o buffer de saída e garante que a mensagem seja exibida na tela.


    if (send(sockfd, client_msg, strlen(client_msg) + 1, 0) < 0) { // envia a mensagem contendo a operação e os números para o servidor
// função send() retorna o número de bytes enviados, ou -1 em caso de erro.
        perror("Error sending client message");
        exit(EXIT_FAILURE);
    }

// chama a função recv_msg() para receber a resposta do servidor
    if(recv_msg(sockfd, recvBuff) < 0) {
            perror("Error receiving request");
            exit(EXIT_FAILURE);
    }

    printf("Resultado recebido: %s\n",recvBuff);
    return 0; //  indica que a execução ocorreu com sucesso
}
