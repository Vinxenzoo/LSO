#ifndef DATASTRUCT_H
#define DATASTRUCT_H

//Used libraries
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

typedef struct ServerStruct //Pascal Case
{
    int fd;
    socklen_t addrlen;
    struct sockaddr_in address;
    unsigned short int opt;

}Server; //Pascal Case

#endif
