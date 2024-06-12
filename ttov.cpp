#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/stat.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include "packet.h"

int main(int argc, char *argv[])
{
    int opt;
    int page;
    const char *source = NULL;
    int input_fd = 0;

    while ((opt = getopt(argc, argv, "i:p:")) != -1)
    {
        switch (opt) 
        {
            case 'i':
                source = optarg;
                break;
            case 'p':
                page = atoi(optarg);
                break;
            default: /* '?' */
                fprintf(stderr, "Usage: %s [-i file] [-p pave]\n", argv[0]);
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
    for(;;)
        if(parser->parse_packet(input_fd, 1)<0) break;

    free(parser);
    return EXIT_SUCCESS;
}