#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "debug.h"
#include "server_socket.h"

// Technically this is incorrect as another protocol could become available but the default case will do the right 
// thing and perform the 'any' case
int ServerSocket::listen(int port, bool v4, bool v6)
{
    char port_s[16];
    int fd = -1;
    int err;

    struct addrinfo hints = {
        ai_flags: AI_PASSIVE | AI_NUMERICSERV,
        ai_family: (!v4 & v6) ? AF_INET6 : (v4 && !v6) ? AF_INET : 0,
        ai_socktype: SOCK_STREAM,
        ai_protocol: IPPROTO_TCP,
    };

    snprintf(port_s,sizeof(port_s),"%d", port);
    socket_count = 0;

    struct addrinfo *result, *ai;

    if((err=getaddrinfo(NULL, port_s, &hints, &result)) != 0 )
    {
        fprintf(stderr, "Socket error: %s\n", gai_strerror(err));
        exit(EXIT_FAILURE);
    }

    for(ai=result; ai; ai=ai->ai_next)
    {
        if((fd = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol)) < 0)
            continue;

        int one = 1;
        setsockopt(fd, IPPROTO_IPV6, IPV6_V6ONLY, &one, sizeof (one));

        if(bind(fd, ai->ai_addr, ai->ai_addrlen)< 0)
        {
            close(fd);
            continue;
        }
        if(::listen(fd, 10)<0)
        {
            close(fd);
            continue;
        }
        sockets[socket_count++] = fd;

        if(socket_count == MAX_SOCKETS)
            break; 
    }

    freeaddrinfo(result);

    return socket_count>0 ? 0 : -1;
}

int ServerSocket::accept()
{
    fd_set fds;
    int nfds = 0;

    if(socket_count <= 0)
        return -1;

    FD_ZERO(&fds);
    for(int i=0; i<socket_count; i++)
    {
        FD_SET(sockets[i], &fds);
        if(sockets[i] > nfds) nfds = sockets[i];
    }

    if((select(nfds+1, &fds, NULL, NULL, NULL)) < 0)
        return -1;

    for(int i = 0; i < socket_count; i++)
        if(FD_ISSET(sockets[i], &fds))
            return ::accept(sockets[i], NULL, NULL);
    
    return -1; // Can't really get here, I think
}

int ServerSocket::read(int fd, uint8_t *buf, int size)
{
    fd_set fds;
    timeval tv = {0};
    int ret;

    FD_ZERO(&fds);
    FD_SET(fd, &fds);

    tv.tv_sec = 0;
    tv.tv_usec = 1;
    if((ret = (select(fd+1, &fds, NULL, NULL, &tv))) != 1)
        return ret;

    return ::read(fd, buf, size);
}
