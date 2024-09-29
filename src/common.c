#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "../headers/common.h"

#define MAX(x, y) (x > y ? x : y)

t_nodeinfo * node_init(char * ipaddr, char * port, char * reg_ipaddr, char * reg_port, int server_sockfd){

    t_nodeinfo * node = (t_nodeinfo *)calloc(1, sizeof(t_nodeinfo));

    node->self_id = -1;
    strcpy(node->self_ipaddr, ipaddr);
    strcpy(node->self_port, port);

    strcpy(node->reg_ipaddr, reg_ipaddr);
    strcpy(node->reg_port, reg_port);

    node->ext_id = -1;
    memset(node->ext_ipaddr, 0, sizeof(node->ext_ipaddr));
    memset(node->ext_port, 0, sizeof(node->ext_port));
    node->ext_tcp_sockfd = -1;

    node->backup_id = -1;
    memset(node->backup_ipaddr, 0, sizeof(node->backup_ipaddr));
    memset(node->backup_port, 0, sizeof(node->backup_port));

    node->int_list = NULL;
    node->n_int = 0;

    node->rout_list = NULL;
    node->n_rout = 0;

    node->names_list = NULL;
    node->n_names = 0;

    node->server_tcp_sockfd = server_sockfd;

    node->net = -1;

    return node;
}

void node_refresh(t_nodeinfo * node){

    node->self_id = -1;

    node->ext_id = -1;
    memset(node->ext_ipaddr, 0, sizeof(node->ext_ipaddr));
    memset(node->ext_port, 0, sizeof(node->ext_port));
    node->ext_tcp_sockfd = -1;

    node->backup_id = -1;
    memset(node->backup_ipaddr, 0, sizeof(node->backup_ipaddr));
    memset(node->backup_port, 0, sizeof(node->backup_port));

    for (t_int * current = node->int_list; current != NULL; current = current->next_int) remove_int(&node->int_list, current->int_id);
    node->int_list = NULL;
    node->n_int = 0;

    for (int i = 0; i < node->n_rout; i++){ free(&node->rout_list[i]); };
    node->rout_list = NULL;
    node->n_rout = 0;

    for (int i = 0; i < node->n_names; i++){ free(&node->names_list[i]); };
    node->names_list = NULL;
    node->n_names = 0; 

    node->net = -1;
}

int get_maxfd(t_nodeinfo * node)
{
    int max_int = -1, mx = -1;
    if( node->n_int == 1){ mx = node->int_list->int_tcp_sockfd; }
    else{
        if(node->int_list != NULL){
            for (t_int * current = node->int_list; current->next_int != NULL; current = current->next_int)
            {
                max_int = MAX(current->int_tcp_sockfd, current->next_int->int_tcp_sockfd);
                if(max_int > mx) mx = max_int;
            }
        }
    }
    mx = MAX(mx, node->server_tcp_sockfd);
    mx = MAX(mx, node->ext_tcp_sockfd);
    return mx;
}

void close_sockets(t_nodeinfo * node){
    if(node->server_tcp_sockfd != -1){ close(node->server_tcp_sockfd); node->server_tcp_sockfd = -1; }
    if(node->ext_tcp_sockfd != -1){ close(node->ext_tcp_sockfd); node->ext_tcp_sockfd = -1; }
    for (t_int * current = node->int_list; current != NULL; current = current->next_int)
    {
        if(current->int_tcp_sockfd != -1){ close(current->int_tcp_sockfd); remove_int(&node->int_list, current->int_id); node->n_int--; }
    }    
}

t_int * create_int(unsigned int intr_id, char * intr_ipaddr, char * intr_port, int intr_tcp_port){
    t_int * new_int = (t_int *)calloc(1, sizeof(t_int));
    new_int->int_id = intr_id;
    strcpy(new_int->int_ipaddr, intr_ipaddr);
    strcpy(new_int->int_port, intr_port);
    new_int->int_tcp_sockfd = intr_tcp_port;
    new_int->next_int = NULL;
    return new_int;
}

void add_int(t_int ** head_ref, unsigned int intr_id, char * intr_ipaddr, char * intr_port, int intr_tcp_port){
    t_int * new_int = create_int(intr_id, intr_ipaddr, intr_port, intr_tcp_port);
    new_int->next_int = *head_ref;
    *head_ref = new_int;
}

void remove_int(t_int ** head_ref, unsigned int intr_id){
    t_int * current = *head_ref;
    t_int * previous = NULL;
    while (current != NULL && current->int_id != intr_id)
    {
        previous = current;
        current = current->next_int;
    }
    if(current == NULL){ fprintf(stderr, "Intern not found!\n"); return; }
    if(previous == NULL){ *head_ref = current->next_int; }
    else{ previous->next_int = current->next_int; }
    
    free(current);    
}

int findIntSock(t_int * head, int id){
    t_int * current = head;
    while (current != NULL) {
        if (current->int_id == id) return current->int_tcp_sockfd;
        current = current->next_int;
    }
    return -1;
}

void print_int(t_int * head){
    printf("Internos: ");
    while (head != NULL)
    {
        printf("%02u %s %s\n", head->int_id, head->int_ipaddr, head->int_port);
        printf("          ");
        head = head->next_int;
    }
}

t_routing * create_rout(int dest, int neighbour){
    t_routing * new_rout = (t_routing *)calloc(1, sizeof(t_routing));
    new_rout->dest = dest;
    new_rout->neighbour = neighbour;
    new_rout->next_rout = NULL;
    return new_rout;
}

void add_rout(t_routing ** head_ref, int dest, int neighbour){
    t_routing * new_rout = create_rout(dest, neighbour);
    new_rout->next_rout = * head_ref;
    * head_ref = new_rout;
}

void remove_rout(t_routing ** head_ref, int id){
    t_routing * current = * head_ref;
    t_routing * previous = NULL;
    while (current != NULL)
    {
        if(current->dest == id || current->neighbour == id) break;
        previous = current; current = current->next_rout; 
    }
    if(current == NULL) return;
    if(previous == NULL){ *head_ref = current->next_rout; }
    else{ previous->next_rout = current->next_rout; }
    free(current);
}

void clearRoutList(t_routing * head){
    t_routing * current = head;
    t_routing * next;
    while (current != NULL)
    {
        next = current->next_rout;
        free(current);
        current = next;
    }    
}

int findRout(t_routing * head, int dest){
    t_routing * current = head;
    while (current != NULL) {
        if (current->dest == dest) return current->neighbour;
        current = current->next_rout;
    }
    return -1;
}

void print_rout(t_routing * head){
    printf(" ___________________\n");
    printf("|         |         |\n");
    printf("| Destino | Vizinho |\n");
    printf("|_________|_________|\n");
    if(head != NULL){
        while (head != NULL)
        {
            printf("|         |         |\n");
            printf("|   %02d    |    %02d   |\n", head->dest, head->neighbour);
            printf("|_________|_________|\n");
            head = head->next_rout;
        }
        printf("\n");
    }
    else{
        printf("|                   |\n");
        printf("|     NO ROUTING    |\n");
        printf("|___________________|\n\n");
    }
}

t_names * create_name(char * name){
    t_names * new_name = (t_names *)calloc(1, sizeof(t_names));
    strcpy(new_name->name, name);
    new_name->next_name = NULL;
    return new_name;
}

void add_name(t_names ** head_ref, char * name){
    t_names * new_name = create_name(name);
    new_name->next_name = * head_ref;
    * head_ref = new_name;
}

void remove_name(t_names ** head_ref, char * name){
    t_names * current = * head_ref;
    t_names * previous = NULL;
    while (current != NULL)
    {
        if(strcmp(current->name, name) == 0)break;
        previous = current; current = current->next_name;
    }
    if(current == NULL){ fprintf(stderr, "Name not found.\n"); return; }
    if( previous == NULL ){ * head_ref = current->next_name; }
    else{ previous->next_name = current->next_name; }
    free(current);    
}

int findName(t_names * head, char * name){
    t_names * current = head;
    while (current != NULL) {
        if (strcmp(current->name, name) == 0) return 0;
        current = current->next_name;
    }
    return -1;
}

void print_names(t_names * head){
    printf(" ______________________________________________________________________________________________________\n");
    printf("|                                                                                                      |\n");
    printf("|     NAMES                                                                                            |\n");
    printf("|______________________________________________________________________________________________________|\n");
    if(head != NULL){
        while (head != NULL)
        {
            int len = strlen(head->name);
            printf("|                                                                                                      |\n");
            printf("| %s%*s |\n", head->name, 100-len, "");
            printf("|______________________________________________________________________________________________________|\n");
            head = head->next_name;
        }
        printf("\n");
    }
    else{
        printf("|                                                                                                      |\n");
        printf("|     NO NAMES YET                                                                                     |\n");
        printf("|______________________________________________________________________________________________________|\n\n");
    }
}