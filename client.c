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



// Utilizada para receber uma mensagem em um socket, caracter a caracter, at� que seja encontrada a marca��o de fim de mensagem (caractere nulo)



int recv_msg(int sk, char* b){

    int i = 0, ret;

    //l endo os caracteres do socket um por um, utilizando a fun��o recv da biblioteca sys/socket.h

    while((ret = recv(sk,&b[i],1,0))>0){

        if(b[i] == '\0'){

            break;

        }

        i++;

    }



    if(ret < 0){ // indicativo de erro

        return ret;

    }



    return i; // Quando a fun��o termina de ler a mensagem, ela retorna o n�mero de caracteres lidos (sem contar com o caractere nulo), que � armazenado em i

}



int main(int argc, char *argv[])

{



// inicializa��o de vari�veis

    int sockfd = 0, n = 0;

    char recvBuff[1024];

    struct sockaddr_in serv_addr; 

    const char client_hello[] = "client";

    char client_msg[20];



    // Checa o n�mero de argumentos necess�rios: execu��o, ip, opera��o, n�mero 1, n�mero 2

    if(argc != 5) 

    {

        printf("\n Uso: %s <ip of server> <operation> <number1> <number2> \n",argv[0]);

        return 1;

    } 



    // Determina as strings referentes a cada par�metro: opera��o, n�mero 1 e n�mero 2

    const char* operation = argv[2]; 

    int number1 = atoi(argv[3]);

    int number2 = atoi(argv[4]);



    // 

    snprintf(client_msg, sizeof(client_msg), "%s %d %d", operation, number1, number2); //  formatar a mensagem com base nas informa��es que o usu�rio passou como argumentos na linha de comando (opera��o, n�mero 1 e n�mero 2) e armazena essa mensagem na vari�vel client_msg



    memset(recvBuff, 0,sizeof(recvBuff)); 

    

    if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)

    {

        perror("socket");

        return 1;

    } 



    memset(&serv_addr, 0, sizeof(serv_addr)); // zera a estrutura de dados serv_addr

    serv_addr.sin_family = AF_INET; // indica o uso do IPv4

    serv_addr.sin_port = htons(5000); // define o n�mero da porta para o socket



// converte o endere�o IP passado como argumento argv[1] para o formato necess�rio para a conex�o

    if(inet_pton(AF_INET, argv[1], &serv_addr.sin_addr)<=0)

    {

        perror("inet_pton");

        return 1;

    } 



// A fun��o connect � usada para estabelecer a conex�o com o servidor -> caso falhe: perror

    if( connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)

    {

    	perror("connect");

       	return 1;

    } 



// envia uma mensagem para o servidor atrav�s do socket -> caso a mensagem n�o possa ser enviada -> perror

    if (send(sockfd, client_hello, strlen(client_hello) + 1, 0) < 0) {

        perror("Error sending client message");

        exit(EXIT_FAILURE);

    }



    printf("Enviando %s \n",client_msg); // imprime a mensagem a ser enviada para o servidor

    fflush(stdout); // limpa o buffer de sa�da e garante que a mensagem seja exibida na tela.





    if (send(sockfd, client_msg, strlen(client_msg) + 1, 0) < 0) { // envia a mensagem contendo a opera��o e os n�meros para o servidor

// fun��o send() retorna o n�mero de bytes enviados, ou -1 em caso de erro.

        perror("Error sending client message");

        exit(EXIT_FAILURE);

    }



// chama a fun��o recv_msg() para receber a resposta do servidor

    if(recv_msg(sockfd, recvBuff) < 0) {

            perror("Error receiving request");

            exit(EXIT_FAILURE);

    }



    printf("Resultado recebido: %s\n",recvBuff);

    return 0; //  indica que a execu��o ocorreu com sucesso

}
