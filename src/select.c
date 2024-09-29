#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include "../headers/common.h"


int select_event(t_nodeinfo * node)
{
    fd_set inputs;
    FD_ZERO(&inputs); // Clear inputs

    //set fds in use
    FD_SET(STDIN_FILENO,&inputs); // Set standard input channel on
    if(node->server_tcp_sockfd > 0) FD_SET(node->server_tcp_sockfd, &inputs);
    if(node->ext_tcp_sockfd > 0) FD_SET(node->ext_tcp_sockfd, &inputs);
    for (t_int * current = node->int_list; current != NULL; current = current->next_int){ if(current->int_tcp_sockfd > 0) FD_SET(current->int_tcp_sockfd, &inputs); }

    int maxfd = get_maxfd(node);
    maxfd = maxfd > STDIN_FILENO ? maxfd : STDIN_FILENO;

    int out_fds = select(maxfd + 1,&inputs,NULL,NULL,NULL);
    if(out_fds < 0){ fprintf(stderr, "Select error! erro: %d\n", errno); exit(1); }

    while(out_fds--)
    {
        if(FD_ISSET(STDIN_FILENO, &inputs)){
            FD_CLR(STDIN_FILENO, &inputs);
            return 1;
        }
        else if(FD_ISSET(node->server_tcp_sockfd, &inputs)){
            FD_CLR(node->server_tcp_sockfd, &inputs);
            return 2;
        }
        else if(FD_ISSET(node->ext_tcp_sockfd, &inputs)){
            FD_CLR(node->ext_tcp_sockfd, &inputs);
            return 3;
        }
        for (t_int * current = node->int_list; current != NULL; current = current->next_int)
        {
            if(FD_ISSET(current->int_tcp_sockfd, &inputs)){
                FD_CLR(current->int_tcp_sockfd, &inputs);
                return 4 + current->int_id;
            }
        }
    }
    
    return 0;
}