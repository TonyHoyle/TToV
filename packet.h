#ifndef PACKET__H
#define PACKET__H

#include <stdint.h>

class PacketParser
{
private:   
    int8_t current_page;
    int8_t desired_page;
    bool active;

    uint8_t deham(uint8_t value);
    uint8_t deham2(uint8_t *values);
    char tohexchar(uint8_t val);

public:
    void set_page(int page);
    int parse_packet(int input_channel, int output_channel);
};

#endif