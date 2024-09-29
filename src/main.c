#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "../headers/server.h"
#include "../headers/common.h"
#include "../headers/processing.h"
#include "../headers/client.h"
#include "../headers/utils.h"
#include "../headers/select.h"

int main(int argc, char * argv[]){

    if(process_invocation(argc, argv) != 0) exit(1);

    int server_sockfd = server_init(strtoui(argv[2]));
    if(server_sockfd == -1){ fprintf(stderr, "ERROR initiating the server.\n"); exit(1); }

    t_nodeinfo * node = node_init(argv[1], argv[2], argv[3], argv[4], server_sockfd);
    if(node == NULL){ fprintf(stderr, "ERROR creating the node.\n"); exit(1); }

    printf(">>> "); fflush(stdout);
    while (1)
    {
        int event = select_event(node);

        // Clear user prompt
        if (event != 1 && event != 0) printf("\x08\x08\x08\x08");

        int result = 0;
        if (event < 4)
        {
            switch (event)
            {
                case 0:
                    break;

                case 1:
                    result = process_user_input(node);
                    break;

                case 2:
                    result = incoming_connection(node);
                    break;

                case 3:
                    result = process_extern_connection(node);
                    break;
                
                default:
                    break;
            }
        }
        else{
            unsigned int int_id = event - 4;
            result = process_intern_connection(node, int_id);
        }
        
        
        if (result < 0) {
            result = process_leave_command(node);
            if(node->server_tcp_sockfd != -1) close(node->server_tcp_sockfd);
            puts("\x1b[31m[!] An error has occurred!\033[m");
            break;
        }
        if(result > 0) break;
        if(event != 0){
            printf(">>> ");
            fflush(stdout);
        }
    }
    exit(1);
}