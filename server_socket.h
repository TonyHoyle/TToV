#ifndef SOCKET__H
#define SOCKET__H

#define MAX_SOCKETS 64

class ServerSocket
{
private:
    int sockets[MAX_SOCKETS];
    int socket_count;

public:
    int listen(int port, bool v4, bool v6);
    int accept();
};

#endif