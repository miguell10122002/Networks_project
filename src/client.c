#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <time.h>

#include "../headers/common.h"
#include "../headers/processing.h"
#include "../headers/utils.h"

#define MAX_NODES 100

int client_init(char * ipaddr, unsigned int port){

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if(sockfd == -1) return -1;

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(ipaddr); // Endereço do servidor
    server_addr.sin_port = htons(port); // Porta do servidor

    int status = connect(sockfd, (struct sockaddr*) &server_addr, sizeof(server_addr));
    if (status == -1) { perror("Erro ao conectar-se ao servidor"); return -2; }

    return sockfd;
}

int send_tcp_message_to_server(int sockfd, char * message){

    printf("mensagem enviada para o server: %s\n", message);
    int status = send(sockfd, message, strlen(message), 0);
    if (status == -1) { perror("Erro ao enviar mensagem para o servidor"); return -1; }

    return 0;
}

int receive_tcp_message_from_server(int sockfd, char * buffer){

    int bytes_received = recv(sockfd, buffer, 1024, 0);
    if (bytes_received == -1) { perror("Erro ao receber mensagem do servidor"); return -1; }
    buffer[bytes_received] = '\0'; // Adiciona um terminador de string

    // printf("Mensagem recebida do servidor: %s\n", buffer);

    return bytes_received;
}

int sendToAllInterns(t_nodeinfo * node, char * message, unsigned int exception){
    t_int * current = node->int_list;
    while (current != NULL)
    {
        if(current->int_id == exception) {current = current->next_int; continue;}
        int snd = send_tcp_message_to_server(current->int_tcp_sockfd, message);
        if( snd == -1 ) return -1;
        current = current->next_int;
    }
    return 0;
}

int establish_connection(t_nodeinfo * node, char * ipaddr, char * port){

    int sockfd = client_init(ipaddr, strtoui(port));
    if( sockfd < 0) return -1;
    node->ext_tcp_sockfd = sockfd;

    char send_to_server_message[64] = "";
    sprintf(send_to_server_message, "NEW %02d %s %s\n", node->self_id, node->self_ipaddr, node->self_port);

    int snd = send_tcp_message_to_server(sockfd, send_to_server_message);
    if( snd == -1 ) return -1;

    char buffer[1024];
    int rcv = receive_tcp_message_from_server(sockfd, buffer); 
    if( rcv == -1) return -1;

    char * server_response = process_tcp_message(node, buffer, -1, -1);
    if(strcmp(server_response, "\0") == 0) return -1;

    printf("Connection successfully established.\n");
    return 0;
}

int client_udp(t_nodeinfo * node, char * ipaddr, char * port, char * message){
    
    int sockfd;
    struct sockaddr_in servaddr;

    // cria o socket UDP
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) { perror("Erro ao criar o socket"); return -1; }

    // define o endereço IP e a porta do servidor UDP
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(strtoui(port));
    if (inet_aton(ipaddr, &servaddr.sin_addr) == 0) { perror("Erro ao converter endereço IP"); return -2; }

    // envia uma mensagem para o servidor
    if (sendto(sockfd, message, strlen(message), 0, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) { perror("Erro ao enviar mensagem"); return -3; }

    char buffer[1024];
    socklen_t len = sizeof(servaddr);
    int n = recvfrom(sockfd, buffer, sizeof(buffer), 0, (struct sockaddr *)&servaddr, &len);
    if (n > 0) {
        buffer[n] = '\0'; // adiciona o caractere nulo ao final do buffer
    } else { printf("Nenhuma mensagem recebida.\n"); }

    if(strncmp(buffer, "NODESLIST ", 10) == 0)
    {   
        int net;
        char id[3] = "";
        sprintf(id, "%02d", node->self_id);

        char *nodes[MAX_NODES]; // array de ponteiros para armazenar as substrings
        int num_nodes = 0; // número de nodes encontradas
        char *token = strtok(buffer, "\n"); // divide a string em substrings usando o caractere '\n'
        sscanf(token, "NODESLIST %d", &net);
        token = strtok(NULL, "\n");
        int * nodes_ids = (int *)calloc(MAX_NODES, sizeof(int));
        int already_taken = 0;
        int temp_id;
        char temp_ip[INET_ADDRSTRLEN], temp_port[6];
        while (token != NULL) {
            if (strncmp(token, id, 2) == 0){ already_taken = 1; }
            sscanf(token, "%d %s %s", &temp_id, temp_ip, temp_port);
            nodes_ids[temp_id] = 1;
            nodes[num_nodes++] = token; // adiciona o ponteiro para a substring atual ao array
            token = strtok(NULL, "\n"); // continua dividindo a string em substrings usando o caractere '\n'
        }

        if(already_taken == 1){
            int found = 0;
            for (int i = 1; i < MAX_NODES; i++){
                if(nodes_ids[i] == 0){ node->self_id = i; printf("\nYour NODE ID was already taken. New NODE ID: %02d.\n", i); found = 1; break;}
            }
            if(found == 0){ fprintf(stderr, "Network full.\n"); return -1;}
        }

        int chosen_id;
        char chosen_ip[INET_ADDRSTRLEN], chosen_port[6];

        if(num_nodes == 0){

            chosen_id = node->self_id;
            strcpy(chosen_ip, node->self_ipaddr);
            strcpy(chosen_port, node->self_port);
            printf("\nNetwork was empty. First NODE in the list.\n");

        }else if(num_nodes == 1){
            printf("\n%s\n", buffer);
            for(int i = 0; i < num_nodes; i++)printf("%s\n", nodes[i]);
            sscanf(nodes[0], "%d %s %s", &chosen_id, chosen_ip, chosen_port);
            printf("\nConnecting to NODE: %02d %s %s\n\n", chosen_id, chosen_ip, chosen_port);

        }else{
            while(1){
                printf("\n%s\n", buffer);
                for(int i = 0; i < num_nodes; i++)printf("%s\n", nodes[i]);
                printf("\nChose the node id to connect: ");
                char input[64];
                fgets(input, 64, stdin);
                input[strlen(input)-1] = '\0';
                if(!strisui(input)){ fprintf(stderr, "NODE ID must be a number (was %s).\n", input); continue; }
                if(strtoui(input) < 0 || strtoui(input) > 99){ fprintf(stderr, "NODE ID must be between 01 and 99 (was %s).\n", input); continue; }
                char node_id[3] = "";
                sprintf(node_id, "%02u", strtoui(input));
                int found = 0;
                for (int i = 0; i < num_nodes; i++)
                {
                    if(strncmp(node_id, nodes[i], 2) == 0){
                        sscanf(nodes[i], "%d %s %s", &chosen_id, chosen_ip, chosen_port);
                        found = 1;
                        break;
                    }
                }
                if(found == 0){ fprintf(stderr, "\nNODE ID not found. Please choose an existing NODE ID from  the following list.\n"); continue; }
                break;                
            }
            printf("\nChosen NODE to connect: %02d %s %s\n\n", chosen_id, chosen_ip, chosen_port);
        }

        if(process_djoin_command(node, net, node->self_id, chosen_id, chosen_ip, chosen_port) != 0) return -1;
        if(chosen_id != node->self_id){
            char message_reg[64] = "";
            sprintf(message_reg, "REG %03u %02u %s %s", net, node->self_id, node->self_ipaddr, node->self_port);

            // envia uma mensagem para o servidor
            if (sendto(sockfd, message_reg, strlen(message_reg), 0, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
                perror("Erro ao enviar mensagem");
                return -3;
            }

            char buffer_reg[1024];
            socklen_t len = sizeof(servaddr);
            int n = recvfrom(sockfd, buffer_reg, sizeof(buffer_reg), 0, (struct sockaddr *)&servaddr, &len);
            if (n > 0) { buffer_reg[n] = '\0'; } 
            else { printf("Nenhuma mensagem recebida.\n"); }
            if(strcmp(buffer_reg, "OKREG") == 0){ printf("\nNODE successfully registered on the network.\n\n"); }
            else if(strcmp(buffer_reg, "OKUNREG") == 0){ printf("\nNODE successfully unregistered from the network.\n\n"); }
        }
    }
    else if(strcmp(buffer, "OKREG") == 0){
        printf("\nNODE successfully registered on the network.\n\n");
    }
    else if(strcmp(buffer, "OKUNREG") == 0){
        printf("\nNODE successfully unregistered from the network.\n\n");
    }
    // fecha o socket
    close(sockfd);
    
    return 0;
}