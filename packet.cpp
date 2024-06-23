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

int PacketParser::decode_page(uint8_t mag, uint8_t *data)
{
    int units = deham(data[0]);
    int tens = deham(data[1]);

    return (mag<<8)+(tens<<4)+units;
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
    struct {
        unsigned erase_page : 1;
        unsigned suppress_header : 1;
        unsigned update_indicator : 1;
        unsigned interrupted_sequence : 1;
        unsigned inhibit_display : 1;
    } flags;

    if(mag==0) mag=8;
 
    current_page = decode_page(mag, line+2);

    if(row == 27 && state==InPage)
    {
        if(mag != desired_page >> 8)
            return state;

        // fasttext
        fastext_page[0] = decode_page(mag, line+3);
        fastext_page[1] = decode_page(mag, line+9);
        fastext_page[2] = decode_page(mag, line+15);
        fastext_page[3] = decode_page(mag, line+21);

        return state;
    }

    if(row > 24)
        return state;

    if(state == PageEnd && current_page != desired_page)
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
        if(mag != desired_page >> 8)
            return state;

        if(state == InPage)
        {
            state = PageEnd;
            return state;
        }

        // Todo: Verify these make sense and act on them
        flags.erase_page = line[5]&0x80 ? 1:0;
        flags.suppress_header = line[8]&2 ? 1:0;
        flags.update_indicator = line[8]&8 ? 1:0;
        flags.interrupted_sequence = line[8]&32 ? 1:0;
        flags.inhibit_display = line[8]&0x80 ? 1:0;

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
            line[3]=(desired_page >> 8)+'0';
            line[4]=((desired_page & 0xff) >> 4)+'0';
            line[5]=(desired_page & 0x0f)+'0';
        }

        if(current_page == desired_page)
        {
            current_row = -1;
            write(output_channel, &clear, 1);
            memcpy(static_line, line, sizeof(line));
            memset(fastext_page, 0, sizeof(fastext_page));
            state = InPage;
        }
        else if(true)
        {
            write(output_channel, &home, 1);
            if(write_line(output_channel, line+2))
                return PageError;
        }
    }

    if(state != InPage)
        return state;

    if(mag != (desired_page>>8))
        return state;

    for(current_row++;current_row < row; current_row++)
        write(output_channel, "\r\n", 2);

    if(write_line(output_channel, line+2))
        return PageError;
 
    return state;
}

void PacketParser::set_page(int page)
{
    desired_page = ((page/100)<<8)+(((page%100)/10)<<4) + (page%10);
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
            desired_page = ((page_char[0]-'0')<<8)+((page_char[1]-'0')<<4)+(page_char[2]-'0');
            current_page = -1;
            page_char_index = 0;
            state = Searching;
        }
    }
    if((key >='A' && key <='D') || (key >= 'a' && key <= 'd'))
    {
        int ix = (key>='a') ? key-'a' : key-'A';
        if(fastext_page[ix] > 0)
        {
            desired_page = fastext_page[ix];
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