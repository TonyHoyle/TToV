#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/stat.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include "packet.h"
#include "server_socket.h"
#include "debug.h"

int main(int argc, char *argv[])
{
    int opt;
    int page = 100;
    const char *source = NULL;
    int input_fd = 0;
    int listen_port = -1;
    bool v6 = true, v4 = true;

    while ((opt = getopt(argc, argv, "64l:i:p:")) != -1)
    {
        switch (opt) 
        {
            case 'i':
                source = optarg;
                break;
            case 'p':
                page = atoi(optarg);
                break;
            case 'l':
                listen_port = atoi(optarg);
                break;
            case '6':
                v4 = false;
                break;
            case '4':
                v6 = false;
                break;
            default: /* '?' */
                fprintf(stderr, "Usage: %s [-i file] [-p page] [-l port] [-6] [-4]\n", argv[0]);
                exit(EXIT_FAILURE);
        }
    }    

    if(source)
    {
        input_fd = open(source, O_RDONLY);
        if(input_fd == -1)
        {
            fprintf(stderr, "Couldn't open %s: %s\n", source, strerror(errno));
            exit(EXIT_FAILURE);
        }
    }

    PacketParser *parser = new PacketParser();

    parser->set_page(page);

    if(listen_port > 0) 
    {
        ServerSocket *socket = new ServerSocket();

        if(socket->listen(listen_port, v4, v6) < 0)
        {
            fprintf(stderr, "Listen error: %s\n", strerror(errno));
            exit(EXIT_FAILURE);            
        }

        signal(SIGCHLD,SIG_IGN);
        for(;;)
        {
            int s = socket->accept();
            if(s < 0)
            {
                fprintf(stderr, "Accept error: %s\n", strerror(errno));
                exit(EXIT_FAILURE);            
                break;
            }
            
            int pid = fork();
            if(pid == -1)
            {
                fprintf(stderr, "Fork error: %s\n", strerror(errno));
                exit(EXIT_FAILURE);            
            }
            if(pid == 0)
            {
                while(parser->parse_packet(input_fd, s) != PageError)
                    ;
            }
        }
    }
    else
    {
        // Oneshot page mode
        while(parser->parse_packet(input_fd, 1) != PageEnd)
            ;
    }

    free(parser);
    return EXIT_SUCCESS;
}