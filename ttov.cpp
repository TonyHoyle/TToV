#include "packet.h"

int main(int argc, char *argv[])
{
    for(;;)
        if(parse_packet(0, 1)<0) break;
    return 0;
}