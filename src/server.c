#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include "../headers/common.h"
#include "../headers/processing.h"

int server_init(unsigned int port){

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY; // Qualquer endereço
    server_addr.sin_port = htons(port); // Porta a ser utilizada

    int status = bind(sockfd, (struct sockaddr*) &server_addr, sizeof(server_addr));
    if (status == -1) { perror("Erro ao vincular o socket ao endereço e porta do servidor"); return -1; }

    int backlog = 5; // Número máximo de conexões pendentes
    status = listen(sockfd, backlog);
    if (status == -1) { perror("Erro ao aguardar conexão de cliente"); return -2; }

    return sockfd;
}

int accept_client_connection(int sockfd){

    struct sockaddr_in client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    int client_sockfd = accept(sockfd, (struct sockaddr*) &client_addr, &client_addr_len);
    if (client_sockfd == -1) { perror("Erro ao aceitar conexão de cliente"); return -1; }

    return client_sockfd;
}

int receive_tcp_message_from_client(int client_sockfd, char * buffer){

    int bytes_received = recv(client_sockfd, buffer, 1024, 0);
    if (bytes_received == -1) { perror("Erro ao receber mensagem do cliente"); return -1; }
    buffer[bytes_received] = '\0'; // Adiciona um terminador de string

    // printf("Mensagem recebida do cliente: %s\n", buffer);

    return bytes_received;
}

int send_tcp_message_to_client(int client_sockfd, char * message){

    int bytes_sent = send(client_sockfd, message, strlen(message), 0);
    if (bytes_sent == -1) { perror("Erro ao enviar mensagem para o cliente"); return -1; }

    return 0;
}

int incoming_connection(t_nodeinfo * node){
    int client_sockfd = accept_client_connection(node->server_tcp_sockfd);
    if(client_sockfd == -1) return -1;

    char buffer[1024];
    int rcv = receive_tcp_message_from_client(client_sockfd, buffer);
    if(rcv == -1){ return -1; }
    else{ if(rcv == 0) return 0; }

    char * send_to_client_message = process_tcp_message(node, buffer, client_sockfd, -1);
    if(strcmp(send_to_client_message, "\0") == 0) return -1;

    int snd = send_tcp_message_to_client(client_sockfd, send_to_client_message);
    if(snd == -1) return -1;

    return 0;
}