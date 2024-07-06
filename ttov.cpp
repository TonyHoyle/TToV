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
#include "services.h"

int main(int argc, char *argv[])
{
    int opt;
    int page = 100;
    const char *source = NULL;
    int input_fd = 0;
    int listen_port = -1;
    bool v6 = true, v4 = true, service = false;

    while ((opt = getopt(argc, argv, "s64l:i:p:")) != -1)
    {
        switch (opt) 
        {
            case 's':
                service = true;
                break;
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
                fprintf(stderr, "Usage: %s [-s] [-i file] [-p page] [-l port] [-6] [-4]\n", argv[0]);
                exit(EXIT_FAILURE);
        }
    }   

    if(service)
    {
        ServicesFile serviceFile;
        if(!serviceFile.read_services("known_services.json"))
        {
            fprintf(stderr, "Couldn't read service file\n");
            exit(EXIT_FAILURE);
        }

        for(auto i = serviceFile.services().begin(); i != serviceFile.services().end(); i++)
        {
            debug("%s\n",i->name.c_str());
            for(auto j = i->services.begin(); j != i->services.end(); j++)
            {
                debug("  %s\n",j->name.c_str());
                for(auto k = j->services.begin(); k != j->services.end(); k++)
                {
                    debug("    %s\n",k->name.c_str());
                }
            }
        }
        exit(EXIT_SUCCESS);
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
                uint8_t c;

                parser->set_page(page);

                while(parser->parse_packet(input_fd, s) != PageError)
                    if(socket->read(s, &c, 1) == 1)
                        parser->key_pressed(c);
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