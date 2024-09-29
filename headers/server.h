#ifndef SERVER_H
#define SERVER_H

#include "common.h"

int server_init(int port);
int accept_client_connection(int sockfd);
int receive_tcp_message_from_client(int client_sockfd);
int send_tcp_message_to_client(int client_sockfd, char * message);
int incoming_connection(t_nodeinfo * node);

#endif