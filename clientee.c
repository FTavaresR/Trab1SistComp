/* Adaptado de https://www.thegeekstuff.com/2011/12/c-socket-programming/?utm_source=feedburner */

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

char* gerar_string_operacao() {
    char op[10];
    int num1, num2;

    // Lê a operação e os números

    printf("Digite a operação desejada e os respectivos numeros: ");
    scanf("%s", op);
    scanf("%d", &num1);
    scanf("%d", &num2);

    // Imprime o resultado

    printf("Operação: %s\n", op);
    printf("Número 1: %d\n", num1);
    printf("Número 2: %d\n", num2);

    char* resultado = malloc(sizeof(char) * 20);
    sprintf(resultado, "%s %d %d", op, num1, num2);  
    return resultado;
}

int main(int argc, char *argv[])
{
    int sockfd = 0, n = 0;
    char recvBuff[1024];
    struct sockaddr_in serv_addr; 
    const char client_hello[] = "client";
    char client_msg[20];

    if(argc != 2)
    {
        printf("\n Usage: %s <ip of server> \n",argv[0]);
        return 1;
    } 

    memset(recvBuff, 0,sizeof(recvBuff));

    /*Cria o Socket */

    if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("socket");
        return 1;
    } 

	/* Configura o IP de destino e porta na estrutura sockaddr_in */

    memset(&serv_addr, 0, sizeof(serv_addr)); 
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(5000); 

    if(inet_pton(AF_INET, argv[1], &serv_addr.sin_addr)<=0)
    {
        perror("inet_pton");
        return 1;
    } 

	/* Conecta ao servidor. */

    if( connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
    	perror("connect");
       	return 1;
    } 

    if (send(sockfd, client_hello, strlen(client_hello) + 1, 0) < 0) {
        perror("Error sending client message");
        exit(EXIT_FAILURE);
    }

    strcpy(client_msg, gerar_string_operacao());
    printf("Sending %s \n",client_msg);
    fflush(stdout);

    if (send(sockfd, client_msg, strlen(client_msg) + 1, 0) < 0) {
        perror("Error sending client message");
        exit(EXIT_FAILURE);
    }

    if(recv_msg(sockfd, recvBuff) < 0) {
            perror("Error receiving request");
            exit(EXIT_FAILURE);
    }

    printf("Resultado recebido: %s\n",recvBuff);
    return 0;
}