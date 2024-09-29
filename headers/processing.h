#ifndef PROCESSING_H
#define PROCESSING_H

#include "common.h"

void user_message_help();
int process_djoin_command(t_nodeinfo * node, int net, int id, int bootid, char * bootip, char * bootport);
int process_join_command(t_nodeinfo * node, int net, int id);
int process_get_command(t_nodeinfo * node, int dest, char * name);
int process_leave_command(t_nodeinfo * node);
void process_st_command(t_nodeinfo * node);
void process_sr_command(t_nodeinfo * node);
void process_sn_command(t_nodeinfo * node);
int process_user_input(t_nodeinfo * node);
char * process_tcp_message(t_nodeinfo * node, char * cleient_message, int client_sockfd, int origem);
int process_extern_disconnection(t_nodeinfo * node);
int process_extern_connection(t_nodeinfo * node);
int process_intern_disconnection(t_nodeinfo * node, unsigned int int_id);
int process_intern_connection(t_nodeinfo * node, unsigned int int_id);

#endif