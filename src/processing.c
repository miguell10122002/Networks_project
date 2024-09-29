#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "../headers/common.h"
#include "../headers/client.h"
#include "../headers/utils.h"
#include "../headers/server.h"

void user_message_help(){
    puts("");
    puts("\t\x1b[4mj\033[moin \x1b[3mnet id\033[m                           -> smtg");
    puts("\t\x1b[4mdj\033[moin \x1b[3mnet id bootid bootIP bootTCP\033[m    -> smtg");
    puts("\t\x1b[4mc\033[mreate \x1b[3mname\033[m                           -> smtg");
    puts("\t\x1b[4md\033[melete \x1b[3mname\033[m                           -> smtg");
    puts("\t\x1b[4mg\033[met \x1b[3mdest name\033[m                         -> smtg");
    puts("\t\033[mshow \x1b[3mtopology \x1b[4m(st)\033[m                    -> smtg");
    puts("\t\033[mshow \x1b[3mnames \x1b[4m(sn)\033[m                       -> smtg");
    puts("\t\033[mshow \x1b[3mrouting \x1b[4m(sr)\033[m                     -> smtg");
    puts("\t\x1b[4ml\033[meave                                 -> leave the net");
    puts("\t\x1b[4me\033[mxit                                  -> exit application");
    puts("");
}

int process_djoin_command(t_nodeinfo * node, int net, int id, int bootid, char * bootip, char * bootport){

    if(!isipaddr(bootip)){ fprintf(stderr, "IP ADDRESS must be a valid IP address (was %s).\n", bootip); return -1; }
    if(id < 0 || id > 99){ fprintf(stderr, "ID must be between 00 and 99 (was %02d).\n", id); return -1;}
    if(bootid < 0 || bootid > 99){ fprintf(stderr, "BOOT ID must be between 00 and 99 (was %02d).\n", bootid); return -1;}
    if(net < 0 || net > 999){ fprintf(stderr, "NET must be between 00 and 999 (was %02d).\n", net); return -1;}

    node->self_id = id;
    node->ext_id = id;
    strcpy(node->ext_ipaddr, node->self_ipaddr);
    strcpy(node->ext_port, node->self_port);
    node->backup_id = id;
    strcpy(node->backup_ipaddr, node->self_ipaddr);
    strcpy(node->backup_port, node->self_port);
    node->net = net;

    if(bootid != id){
        node->ext_id = bootid;
        strcpy(node->ext_ipaddr, bootip);
        strcpy(node->ext_port, bootport);

        int connection = establish_connection(node, bootip, bootport);
        if(connection == -1) return -1;
    }
    else{
        char message[64] = "";
        sprintf(message, "REG %03u %02u %s %s", net, node->self_id, node->self_ipaddr, node->self_port);

        if(client_udp(node, node->reg_ipaddr, node->reg_port, message) != 0){
            fprintf(stderr, "ERROR sending udp message!\n");
            return -1;
        }
    }
    if(bootid != id ) add_rout(&node->rout_list, bootid, bootid);
    return 0;
}

int process_join_command(t_nodeinfo * node, int net, int id){

    if(id < 1 || id > 99){ fprintf(stderr, "ID must be between 01 and 99 (was %d).\n", id); return 0;}
    if(net < 1 || net > 999){ fprintf(stderr, "NET must be between 01 and 999 (was %d).\n", net); return 0;}

    node->self_id = id;
    node->net = net;

    char message[64] = "";
    sprintf(message, "NODES %03d", net);

    if(client_udp(node, node->reg_ipaddr, node->reg_port, message) != 0){
        fprintf(stderr, "ERROR sending udp message!\n");
        return -1;
    }

    return 0;
}

int process_get_command(t_nodeinfo * node, int dest, char * name){
    char message[128] = "";
    sprintf(message, "QUERY %02d %02d %s\n", dest, node->self_id, name);

    int id = findRout(node->rout_list, dest);
    if( id != -1 ){
        int sock = 0;
        if(id == node->ext_id) sock = node->ext_tcp_sockfd;
        else{ sock = findIntSock(node->int_list, id); }
        int snd = send_tcp_message_to_server(sock, message);
        if( snd == -1 ) return -1;
    }
    else{
        if(node->ext_tcp_sockfd != -1){
            add_rout(&node->rout_list, node->ext_id, node->ext_id);
            int snd = send_tcp_message_to_server(node->ext_tcp_sockfd, message);
            if(snd == -1) return -1;
        }
        for (t_int * current = node->int_list; current != NULL; current = current->next_int)
        {
            add_rout(&node->rout_list, current->int_id, current->int_id);
            int snd = send_tcp_message_to_server(current->int_tcp_sockfd, message);
            if(snd == -1) return -1;
        }        
    }
    return 0;
}

void process_st_command(t_nodeinfo * node){

    if(node->self_id != -1){
        printf("===== SHOW TOPOLOGY =====\n\n");
        printf("My Node: %02d %s %s\n", node->self_id, node->self_ipaddr, node->self_port);
        printf("Externo: %02d %s %s\n", node->ext_id, node->ext_ipaddr, node->ext_port);
        printf("Backup: %02d %s %s\n", node->backup_id, node->backup_ipaddr, node->backup_port);
        print_int(node->int_list);
        printf("\n\n");
        printf("=========================\n");
    }
    else{
        printf("===== SHOW TOPOLOGY =====\n\n\n\n");
        printf(" NODE NOT IN ANY NETWORK");
        printf("\n\n\n\n");
        printf("=========================\n");
    }
}

void process_sr_command(t_nodeinfo * node){ print_rout(node->rout_list); }

void process_cr_command(t_nodeinfo * node){ clearRoutList(node->rout_list); node->rout_list = NULL; }

void process_sn_command(t_nodeinfo * node){ print_names(node->names_list); }

int process_leave_command(t_nodeinfo * node){

    if(node->self_id == -1) return 0;
    //mensagem udp de unreg
    char message[64] = "";
    sprintf(message, "UNREG %03u %02u", node->net, node->self_id);

    if(client_udp(node, node->reg_ipaddr, node->reg_port, message) != 0){
        fprintf(stderr, "ERROR sending udp message!\n");
        return -1;
    }

    close_sockets(node);

    node_refresh(node);

    return 0;
}

int process_user_input(t_nodeinfo * node){
    
    char input[128];
    fgets(input, 128, stdin);

    if (strncmp(input, "join ", 5) == 0 || strncmp(input, "j ", 2) == 0)
    {
        if(node->self_id != -1){ fprintf(stderr, "NODE already in a network\n\n"); return 0; }
        int net, id;
        char * token = strchr(input, ' ');
        if(token && sscanf(token+1, "%u %u\n", &net, &id) == 2){
            if(process_join_command(node, net, id) != 0) return -1;
            return 0;
        }else{
            puts("Invalid format.\nUsage: \x1b[4mj\033[moin \x1b[3mnet id\033[m");
        }
        return 0;
    }
    if(strncmp(input, "djoin ", 6) == 0 || strncmp(input, "dj ", 3) == 0)
    {
        if(node->self_id != -1){ fprintf(stderr, "NODE already in a network\n\n"); return 0; }
        int net, id, bootid;
        char bootip[INET_ADDRSTRLEN] = "", boottcp[6] = "";
        char * token = strchr(input, ' ');
        if(token && sscanf(token+1, "%u %u %u %15s %s\n", &net, &id, &bootid, bootip, boottcp) == 5){
            bootip[15] = '\0';
            if(process_djoin_command(node, net, id, bootid, bootip, boottcp) != 0) return -1;
            return 0;
        }else{ puts("Invalid format.\nUsage: \x1b[4mdj\033[moin \x1b[3mnet id bootid bootIP bootTCP\033[m"); }
        return 0;
    }
    if(strncmp(input, "create ", 7) == 0 || strncmp(input, "c ", 2) == 0)
    {
        if(node->self_id == -1){ fprintf(stderr, "\nNODE not in a network.\nJoin a network to create names.\n\n"); return 0; }
        char name[101] = "";
        char * token = strchr(input, ' ');
        if(token && sscanf(token+1, "%s\n", name) == 1){
            add_name(&node->names_list, name);
            printf("\nSuccessfully created name: %s\n\n", name);
            return 0;
        }else{
            puts("Invalid format.\nUsage: \x1b[4mc\033[mreate \x1b[3mname\033[m");
        }
        return 0;
    }
    if(strncmp(input, "delete ", 7) == 0 || strncmp(input, "d ", 2) == 0)
    {
        if(node->self_id == -1){ fprintf(stderr, "\nNODE not in a network.\nJoin a network to delete names.\n\n"); return 0; }
        char name[101] = "";
        char * token = strchr(input, ' ');
        if(token && sscanf(token+1, "%s\n", name) == 1){
            remove_name(&node->names_list, name);
            printf("\nSuccessfully deleted name: %s\n\n", name);
            return 0;
        }else{
            puts("Invalid format.\nUsage: \x1b[4md\033[melete \x1b[3mname\033[m");
        }
        return 0;
    }
    if(strncmp(input, "get ", 4) == 0 || strncmp(input, "g ", 2) == 0)
    {
        if(node->self_id == -1){ fprintf(stderr, "\nNODE not in a network.\nJoin a network to get names.\n\n"); return 0; }
        int dest;
        char name[128] = "";
        char * token = strchr(input, ' ');
        if(token && sscanf(token+1, "%d %s", &dest, name) == 2){
            if(process_get_command(node, dest, name) != 0) return -1;
            return 0;
        }else{
            puts("Invalid format.\nUsage: \x1b[4mg\033[met \x1b[3mdest name\033[m");
        }
        return 0;
    }
    if(strcmp(input, "show topology\n") == 0 || strcmp(input, "st\n") == 0)
    {
        if(node->self_id != -1){ process_st_command(node); }
        else{ fprintf(stderr, "\nNODE not in a network.\nJoin a network first to have a topology.\n\n"); }
        return 0;
    }
    if(strcmp(input, "show names\n") == 0 || strcmp(input, "sn\n") == 0)
    {
        if(node->self_id != -1){ process_sn_command(node); }
        else{ fprintf(stderr, "\nNODE not in a network.\nJoin a network first to have a name table.\n\n"); }
        return 0;
    }
    if(strcmp(input, "show routing\n") == 0 || strcmp(input, "sr\n") == 0)
    {
        if(node->self_id != -1){ process_sr_command(node); }
        else{ fprintf(stderr, "\nNODE not in a network.\nJoin a network first to have an expedition table.\n\n"); }
        return 0;
    }
    if(strcmp(input, "leave\n") == 0 || strcmp(input, "l\n") == 0)
    {   
        if(node->self_id == -1){ fprintf(stderr, "\nNODE already not in a network.\n\n"); }
        else{ if(process_leave_command(node) != 0) return -1; }
        return 0;
    }
    if(strcmp(input, "exit\n") == 0 || strcmp(input, "e\n") == 0)
    {
        if(process_leave_command(node) != 0) return -1;
        if(node->server_tcp_sockfd != -1) close(node->server_tcp_sockfd);
        printf("success in exiting.\n");
        return 1;
    }
    if(strcmp(input, "clear routing\n") == 0 || strcmp(input, "cr\n") == 0)
    {
        if(node->rout_list == NULL){ fprintf(stderr, "\nROUTING LIST already empty.\n\n"); }
        return 0;
    }
    
    size_t input_len = strlen(input);
    if(input_len > 0) input[input_len-1] = '\0';
    if(input_len == 1) return 0;

    printf("Invalid command \"%s\". Available commands:\n", input);
    user_message_help();
    return 0;
}

char * process_tcp_message(t_nodeinfo * node, char * message, int client_sockfd, int origem){

    static char server_message[128] = "";
    if(strncmp(message, "NEW ", 4) == 0 && client_sockfd != -1){

        int id;
        char ip[INET_ADDRSTRLEN], port[6];
        char * token = strchr(message, ' ');
        sscanf(token+1, "%d %s %s\n", &id, ip, port);

        if(node->self_id == node->ext_id){
            node->ext_id = id;
            strcpy(node->ext_ipaddr, ip);
            strcpy(node->ext_port, port);
            node->ext_tcp_sockfd = client_sockfd;

            node->backup_id = node->self_id;
            strcpy(node->backup_ipaddr, node->self_ipaddr);
            strcpy(node->backup_port, node->self_port);
        }
        else{
            add_int(&node->int_list, id, ip, port, client_sockfd);
            node->n_int++;
        }
        add_rout(&node->rout_list, id, id);
        sprintf(server_message, "EXTERN %02u %s %s\n", node->ext_id, node->ext_ipaddr, node->ext_port);
    }
    else if (strncmp(message, "EXTERN ", 7) == 0){

        int id;
        char ip[INET_ADDRSTRLEN], port[6];
        char * token = strchr(message, ' ');
        sscanf(token+1, "%d %s %s\n", &id, ip, port);

        node->backup_id = id;
        strcpy(node->backup_ipaddr, ip);
        strcpy(node->backup_port, port);
        sprintf(server_message, "OK\n");

        add_rout(&node->rout_list, id, node->ext_id);
    }
    else if(strncmp(message, "QUERY ", 6) == 0 && client_sockfd != -1){
        
        int dest, orig;
        char name[101];
        char * token = strchr(message, ' ');
        sscanf(token + 1, "%d %d %s\n", &dest, &orig, name);

        // if(findRout(node->rout_list, orig) == -1) add_rout(&node->rout_list, orig, origem);
        // if(findRout(node->rout_list, origem) == -1) add_rout(&node->rout_list, origem, origem);

        if(dest == node->self_id){
            if(findName(node->names_list, name) == 0){ sprintf(server_message, "CONTENT %d %d %s\n", orig, dest, name); }
            else{ sprintf(server_message, "NOCONTENT %d %d %s\n", orig, dest, name); }
            int snd = send_tcp_message_to_client(client_sockfd, server_message);
            if( snd == -1 ) {strcpy(server_message, "\0"); return server_message; }
        }
        else{
            int id = findRout(node->rout_list, dest);
            if( id != -1 ){
                int sock = 0;
                if(id == node->ext_id) sock = node->ext_tcp_sockfd;
                else{ sock = findIntSock(node->int_list, id); }
                int snd = send_tcp_message_to_server(sock, message);
                if( snd == -1 ) {strcpy(server_message, "\0"); return server_message; }
            }
            else{
                if(node->ext_tcp_sockfd != -1 && origem != node->ext_id){
                    int snd = send_tcp_message_to_server(node->ext_tcp_sockfd, message);
                    if(snd == -1) {strcpy(server_message, "\0"); return server_message; }
                }
                t_int * current = node->int_list;
                while (current != NULL)
                {
                    if(current->int_id == origem) {current = current->next_int; continue;}
                    int snd = send_tcp_message_to_server(current->int_tcp_sockfd, message);
                    if(snd == -1) {strcpy(server_message, "\0"); return server_message; }
                    current = current->next_int;
                }      
            }
        }
        sprintf(server_message, "OK\n");
    }
    else if(strncmp(message, "CONTENT ", 8) == 0){
        int dest, orig;
        char name[101];
        char * token = strchr(message, ' ');
        sscanf(token + 1, "%d %d %s\n", &dest, &orig, name);

        if(findRout(node->rout_list, orig) == -1) add_rout(&node->rout_list, orig, origem);

        if(dest == node->self_id){ 
            printf("\x08\x08\x08\x08");
            printf("\nCONTENT FOUND.\n\n");
            if(findRout(node->rout_list, orig) == -1) add_rout(&node->rout_list, orig, origem);
        }
        else{
            int id = findRout(node->rout_list, dest);
            int sock = 0;
            if(id == node->ext_id) sock = node->ext_tcp_sockfd;
            else{ sock = findIntSock(node->int_list, id); }
            int snd = send_tcp_message_to_server(sock, message);
            if( snd == -1 ) {strcpy(server_message, "\0"); return server_message; }
        }
        sprintf(server_message, "OK\n");        
    }
    else if(strncmp(message, "NOCONTENT ", 10) == 0){
        int dest, orig;
        char name[101];
        char * token = strchr(message, ' ');
        sscanf(token + 1, "%d %d %s\n", &dest, &orig, name);

        if(findRout(node->rout_list, orig) == -1) add_rout(&node->rout_list, orig, origem);

        if(dest == node->self_id){ 
            printf("\x08\x08\x08\x08");
            printf("\nCONTENT NOT FOUND.\n\n");
            if(findRout(node->rout_list, orig) == -1) add_rout(&node->rout_list, orig, origem);
        }
        else{
            int id = findRout(node->rout_list, dest);
            int sock = 0;
            if(id == node->ext_id) sock = node->ext_tcp_sockfd;
            else{ sock = findIntSock(node->int_list, id); }
            int snd = send_tcp_message_to_server(sock, message);
            if( snd == -1 ) {strcpy(server_message, "\0"); return server_message; }
        }
        sprintf(server_message, "OK\n"); 
    }
    else if(strncmp(message, "WITHDRAW ", 9) == 0){
        int id;
        char * token = strchr(message, ' ');
        sscanf(token + 1, "%d\n", &id);

        remove_rout(&node->rout_list, id);

        if(node->ext_tcp_sockfd != -1 && node->ext_id != origem){
            int snd = send_tcp_message_to_server(node->ext_tcp_sockfd, message);
            if( snd == -1 ) {strcpy(server_message, "\0"); return server_message; }
        }

        for (t_int * current = node->int_list; current != NULL; current = current->next_int)
        {
            if(current->int_id == origem) continue;
            int snd = send_tcp_message_to_server(current->int_tcp_sockfd, message);
            if(snd == -1) {strcpy(server_message, "\0"); return server_message; }
        }
    }
    else{
        printf("mensagem errada ou não processada");
        strcpy(server_message, "\0");
    }
    
    return server_message;
}

int process_extern_disconnection(t_nodeinfo * node){

    char withdraw_message[64] = "";
    sprintf(withdraw_message, "WITHDRAW %02d\n", node->ext_id);
    t_int * current = node->int_list;
    while (current != NULL)
    {
        int snd = send_tcp_message_to_server(current->int_tcp_sockfd, withdraw_message);
        if( snd == -1 ) {return -1;}
        current = current->next_int;
    }

    if(node->backup_id != node->self_id){
        node->ext_id = node->backup_id;
        strcpy(node->ext_ipaddr, node->backup_ipaddr);
        strcpy(node->ext_port, node->backup_port);
        close(node->ext_tcp_sockfd);
        node->ext_tcp_sockfd = -1;

        int connection = establish_connection(node, node->backup_ipaddr, node->backup_port);
        if(connection == -1) return -1;
        
        char message[64] = "";
        sprintf(message, "EXTERN %02u %s %s\n", node->ext_id, node->ext_ipaddr, node->ext_port);
        
        if(sendToAllInterns(node, message, 0) != 0) return -1;
    }
    else if(node->n_int != 0){
        node->ext_id = node->int_list->int_id;
        strcpy(node->ext_ipaddr, node->int_list->int_ipaddr);
        strcpy(node->ext_port, node->int_list->int_port);
        close(node->ext_tcp_sockfd);
        node->ext_tcp_sockfd = node->int_list->int_tcp_sockfd;

        char message[64] = "";
        sprintf(message, "EXTERN %02u %s %s\n", node->ext_id, node->ext_ipaddr, node->ext_port);

        if(sendToAllInterns(node, message, 0) != 0) return -1;

        remove_int(&node->int_list, node->int_list->int_id);
        node->n_int--;
    }
    else{
        node->ext_id = node->self_id;
        strcpy(node->ext_ipaddr, node->self_ipaddr);
        strcpy(node->ext_port, node->self_port);
        close(node->ext_tcp_sockfd);
        node->ext_tcp_sockfd = -1;
    }

    return 0;
}

int process_extern_connection(t_nodeinfo * node){

    char buffer[1024];
    int rcv = receive_tcp_message_from_server(node->ext_tcp_sockfd, buffer);
    if( rcv == -1 ) return -1;
    else if( rcv == 0){ int result = process_extern_disconnection(node); return result;} //process disconnect
    else{
        char * server_response = process_tcp_message(node, buffer, node->ext_tcp_sockfd, node->ext_id);
        if(strcmp(server_response, "\0") == 0) return -1;
        return 0; //process other messages
    }

    return 0;
}

int process_intern_disconnection(t_nodeinfo * node, unsigned int int_id){

    char withdraw_message[64] = "";
    sprintf(withdraw_message, "WITHDRAW %02d\n", node->ext_id);

    int snd = send_tcp_message_to_server(node->ext_tcp_sockfd, withdraw_message);
    if( snd == -1 ) return -1;

    for (t_int * current = node->int_list; current != NULL; current = current->next_int)
    {
        if(current->int_id == int_id){close(current->int_tcp_sockfd); continue;}
        snd = send_tcp_message_to_server(current->int_tcp_sockfd, withdraw_message);
        if( snd == -1 ) return -1;
    }

    remove_int(&node->int_list, int_id);
    node->n_int--;
    return 0;
}

int process_intern_connection(t_nodeinfo * node, unsigned int int_id){

    char buffer[1024];
    int intsock = findIntSock(node->int_list, int_id);
    int rcv = receive_tcp_message_from_server(intsock, buffer);
    if( rcv == -1 ) return -1;
    else if( rcv == 0){ int result = process_intern_disconnection(node, int_id); return result; }
    else{
        char * server_response = process_tcp_message(node, buffer, intsock, int_id);
        if(strcmp(server_response, "\0") == 0) return -1;
        return 0; //process other messages
    }

    return 0;
}