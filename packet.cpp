#include <unistd.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>
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

    for(int l=0,r; l<sizeof(line); l+=r)
    {
        r = read(input_channel, line+l, sizeof(line)-l);
        if(r<=0)
            return PageError;
    }

    uint8_t mrag = deham2(line); 
    uint8_t mag = mrag&7;
    uint8_t row = mrag>>3;

    if(mag==0) mag=8;

    if(row > 23)
    {   
        // Byte 12, bit 8 Data addressed to rows 1 to 24 is not to be displayed.
        // Handle things like fasttext
        return state;
    }

    if(state == PageEnd)
    {
        bool draw = false;
        if(page_char_index > 0)
        {
            memset(static_line+6, 32, 4);
            static_line[2]='P';
            static_line[3]=page_char[0];
            static_line[4]=page_char[1];
            static_line[5]=page_char[2];
            draw = true;
        }
        if((row==0) && (memcmp(line+32, static_line+32, 10) != 0))
        {
            memcpy(static_line+32, line+32, 10); // time
            draw = true;
        }
        if(draw)
        {
            write(output_channel, &home, 1);
            if(write_line(output_channel, static_line+2))
                return PageError;
        }

        return state;
    }

    if(row == 0)
    {
        if(mag != desired_page / 100)
            return state;

        if(state == InPage)
        {
            state = PageEnd;
            return state;
        }

        int units = deham(line[2]);
        int tens = deham(line[3]);

        if(units > 9 || tens > 9)
            return state;

        memset(line+6, 32, 4);
        line[2]='P';
        if(page_char_index>0)
        {
            line[3]=page_char[0];
            line[4]=page_char[1];
            line[5]=page_char[2];
        }
        else
        {
            line[3]=(desired_page / 100)+'0';
            line[4]=((desired_page % 100) / 10)+'0';
            line[5]=(desired_page % 10)+'0';
        }

        current_page = (mag*100)+(tens*10)+units;

        if(current_page == desired_page)
        {
            state = InPage;
            current_row = -1;
            write(output_channel, &clear, 1);
            memcpy(static_line, line, sizeof(line));
        }
        else 
        {
            write(output_channel, &home, 1);
            if(write_line(output_channel, line+2))
                return PageError;
        }
    }

    if(state != InPage)
        return state;

    if(mag != (desired_page/100))
        return state;

    for(current_row++;current_row < row; current_row++)
        write(output_channel, "\r\n", 2);

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