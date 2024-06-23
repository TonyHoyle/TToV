#ifndef PACKET__H
#define PACKET__H

#include <stdint.h>

enum PageState { Searching, InPage, PageEnd, PageError };

class PacketParser
{
private:   
    int current_page;
    int desired_page;
    int current_row;
    PageState state;
    int page_char_index;
    char page_char[3];
    uint8_t static_line[42];
    int fastext_page[4];

    uint8_t deham(uint8_t value);
    uint8_t deham2(uint8_t *values);
    char tohexchar(uint8_t val);
    int decode_page(uint8_t mag, uint8_t *data);
    int write_line(int output_channel, const uint8_t *line);

public:
    void set_page(int page);
    PageState parse_packet(int input_channel, int output_channel);
    void key_pressed(char key);
};

#endif