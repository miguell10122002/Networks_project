#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>

unsigned int strtoui(const char *str)
{
    size_t len = strlen(str);
    unsigned int result = 0;
    unsigned int power = 1;
    for (size_t i = 0; i < len; i++, power *= 10)
        result += (unsigned int)(str[len-i-1] - '0') * power;
    return result;
}

int strisui(const char *str)
{
    size_t len = strlen(str);
    for (size_t i = 0; i < len; i++) {
        if (str[i] > '9' || str[i] < '0')
            return 0;
    }
    return 1;
}

int isipaddr(const char *str)
{
    struct sockaddr_in sa;
    int result = inet_pton(AF_INET, str, &(sa.sin_addr));
    return result > 0;
}

int process_invocation(int argc, char ** inputsv){

    if(argc != 5){ fprintf(stderr, "Invalid invocation of the program!\n"); return -1;}

    if(!isipaddr(inputsv[1])){
        fprintf(stderr, "IP ADDRESS must be a valid IP address (was %s).\n", inputsv[1]);
        return -1;
    }

    if(!strisui(inputsv[2])){
        fprintf(stderr, "TCP PORT must be a number (was %s).", inputsv[2]);
        return -1;
    }

    if(!isipaddr(inputsv[3])){
        fprintf(stderr, "REG IP ADDRESS must be a valid IP address (was %s).\n", inputsv[3]);
        return -1;
    }

    if(!strisui(inputsv[4])){
        fprintf(stderr, "REG UDP PORT must be a number (was %s).", inputsv[4]);
        return -1;
    }

    return 0;
}