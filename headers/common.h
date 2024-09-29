#ifndef COMMON_H
#define COMMON_H

#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

typedef struct t_names
{
    char name[101];
    struct t_names * next_name;
}t_names;


typedef struct t_routing
{
    int dest;
    int neighbour;
    struct t_routing * next_rout;
}t_routing;


typedef struct t_int
{
    unsigned int int_id;
    char int_ipaddr[INET_ADDRSTRLEN];
    char int_port[6];
    int int_tcp_sockfd;
    struct t_int * next_int;

}t_int;

typedef struct t_nodeinfo
{
    int self_id;
    char self_ipaddr[INET_ADDRSTRLEN];
    char self_port[6];

    char reg_ipaddr[INET_ADDRSTRLEN];
    char reg_port[6];

    int ext_id;
    char ext_ipaddr[INET_ADDRSTRLEN];
    char ext_port[6];
    int ext_tcp_sockfd;

    int backup_id;
    char backup_ipaddr[INET_ADDRSTRLEN];
    char backup_port[6];

    t_int * int_list;
    int n_int;

    t_routing * rout_list;
    int n_rout;

    t_names * names_list;
    int n_names;

    int server_tcp_sockfd;

    int net;

}t_nodeinfo;

t_nodeinfo * node_init(char * ipaddr, char * port, char * reg_ipaddr, char * reg_port, int server_sockfd);
void node_refresh(t_nodeinfo * node);
int get_maxfd(t_nodeinfo * node);
void close_sockets(t_nodeinfo * node);

t_int * create_int(unsigned int intr_id, char * intr_ipaddr, char * intr_port, int intr_tcp_port);
void add_int(t_int ** head_ref, unsigned int intr_id, char * intr_ipaddr, char * intr_port, int intr_tcp_port);
void remove_int(t_int ** head_ref, unsigned int intr_id);
int findIntSock(t_int * head, int id);
void print_int(t_int * head);

t_routing * create_rout(int dest, int neighbour);
void add_rout(t_routing ** head_ref, int dest, int neighbour);
void remove_rout(t_routing ** head_ref, int id);
void clearRoutList(t_routing * head);
int findRout(t_routing * head, int dest);
void print_rout(t_routing * head);

t_names * create_name(char * name);
void add_name(t_names ** head_ref, char * name);
void remove_name(t_names ** head_ref, char * name);
int findName(t_names * head, char * name);
void print_names(t_names * head);

#endif