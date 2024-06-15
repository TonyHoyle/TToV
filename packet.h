#ifndef PACKET__H
#define PACKET__H

#include <stdint.h>

enum PageState { Searching, InPage, PageEnd, PageError };

class PacketParser
{
private:   
    int8_t current_page;
    int8_t desired_page;
    PageState state;

    uint8_t deham(uint8_t value);
    uint8_t deham2(uint8_t *values);
    char tohexchar(uint8_t val);

public:
    void set_page(int page);
    PageState parse_packet(int input_channel, int output_channel);
};

#endif