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

int main(int argc, char *argv[])
{
    int sockfd = 0, n = 0;
    char recvBuff[1024];
    struct sockaddr_in serv_addr; 
    const char client_hello[] = "client";
    char client_msg[20];

    // Check for the correct number of arguments
    if(argc != 5)
    {
        printf("\n Usage: %s <ip of server> <operation> <number1> <number2> \n",argv[0]);
        return 1;
    } 

    // Parse the operation and numbers from command line arguments
    const char* operation = argv[2];
    int number1 = atoi(argv[3]);
    int number2 = atoi(argv[4]);

    // Build the client message
    snprintf(client_msg, sizeof(client_msg), "%s %d %d", operation, number1, number2);

    memset(recvBuff, 0,sizeof(recvBuff));

    if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("socket");
        return 1;
    } 

    memset(&serv_addr, 0, sizeof(serv_addr)); 
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(5000); 

    if(inet_pton(AF_INET, argv[1], &serv_addr.sin_addr)<=0)
    {
        perror("inet_pton");
        return 1;
    } 

    if( connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
    	perror("connect");
       	return 1;
    } 

    if (send(sockfd, client_hello, strlen(client_hello) + 1, 0) < 0) {
        perror("Error sending client message");
        exit(EXIT_FAILURE);
    }

    printf("Enviando %s \n",client_msg);
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
