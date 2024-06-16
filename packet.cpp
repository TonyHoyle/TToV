#include <unistd.h>
#include <string.h>
#include <stdint.h>
#include "packet.h"
#include "debug.h"

uint8_t PacketParser::deham(uint8_t value)
{
    return ((value&0x80)>>4)|((value&0x20)>>3)|((value&0x08)>>2)|((value&0x02)>>1);
}

uint8_t PacketParser::deham2(uint8_t *values)
{
    return (deham(values[1]) << 4) | deham(values[0]);
}

char PacketParser::tohexchar(uint8_t val)
{
    return (val>9)?val+'0'+7:val+'0';
}

PageState PacketParser::parse_packet(int input_channel, int output_channel)
{
    uint8_t line[42];
    const char home = 30;
    const char clear = 12;

    if(read(input_channel, line, sizeof(line))!=sizeof(line))
      return PageError;

    uint8_t mrag = deham2(line); 
    uint8_t mag = mrag&7;
    uint8_t row = mrag>>3;

    // PageEnd is a special state returned to the outer layer, and not important here
    if(state == PageEnd) state = Idle;

    if(mag==0) mag=8;

    if(row > 23)
    {
        // HanByte 12, bit 8 Data addressed to rows 1 to 24 is not to be displayed.dle things like fasttext
        return state;
    }

    if(row == 0)
    {
        // Header row
        int units = deham(line[2]);
        int tens = deham(line[3]);

        current_page = (mag*100)+(tens*10)+units;

        memset(line+2, 32, 8);
        line[2]='P';
        if(page_char_index == 0)
        {
            line[3]=tohexchar(mag);
            line[4]=tohexchar(tens);
            line[5]=tohexchar(units);
        }
        else
        {
            line[3]=page_char[0];
            line[4]=page_char[1];
            line[5]=page_char[2];
        }

        if(current_page == desired_page)
        {
            state = InPage;
            write(output_channel, &clear, 1);
            if(write_line(output_channel, line+2))
                return PageError;
            return InPage;
        }
        else    
        {
            state = (state == InPage) ? PageEnd : Searching;
            write(output_channel, &home, 1);
        }
    }

    if(state != InPage)
        return state;

    if(mag != (current_page/100))
        return state;

    if(write_line(output_channel, line+2))
        return PageError;
 
    return state;
}

void PacketParser::set_page(int page)
{
    desired_page = page;
    current_page = -1;
    page_char_index = 0;
    memset(page_char, 32, 3);
}

void PacketParser::key_pressed(char key)
{
    if(key >= '0' && key <='9')
    {
        if(page_char_index == 0)
            memset(page_char, 32, 3);
            
        page_char[page_char_index++] = key;

        if(page_char_index == 3)
        {
            desired_page = 100*(page_char[0]-'0')+10*(page_char[1]-'0')+(page_char[2]-'0');
            current_page = -1;
            page_char_index = 0;
            state = Searching;
        }
    }
}

int PacketParser::write_line(int output_channel, const uint8_t *line)
{
    for(int n=0; n<40; n++)
    {
        char c[2];
        c[0] = line[n] & 0x7f;
        if(c[0] < 32)
        {
            c[1] = c[0] + 0x40;
            c[0] = 27;
            if(write(output_channel, c, 2) != 2)
                return -1;
        }
        else    
            if(write(output_channel, c, 1) != 1)
                return -1;
    }
    return 0;
}