#ifndef CLIENT_H
#define CLIENT_H

#include "common.h"

int client_init(char * ipaddr, unsigned int port);
int send_tcp_message_to_server(int sockfd, char * message);
int receive_tcp_message_from_server(int sockfd, char * buffer);
int sendToAllInterns(t_nodeinfo * node, char * message, int exception);
int establish_connection(t_nodeinfo * node, char * ipaddr, char * port);
int client_udp(t_nodeinfo * node, char * ipaddr, char * port, char * message);

#endif