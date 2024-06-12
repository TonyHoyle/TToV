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

int PacketParser::parse_packet(int input_channel, int output_channel)
{
    uint8_t line[42];
    if(read(input_channel, line, sizeof(line))!=sizeof(line))
      return -1;

    uint8_t mrag = deham2(line); 
    uint8_t mag = mrag&7;
    uint8_t row = mrag>>3;

    if(mag==0) mag=8;

    if(row > 23)
    {
        // HanByte 12, bit 8 Data addressed to rows 1 to 24 is not to be displayed.dle things like fasttext
        return 0;
    }

    if(row == 0)
    {
        // Header row
        int units = deham(line[2]);
        int tens = deham(line[3]);

        current_page = mag*100+tens*10+units;

        memset(line+2, 32, 8);
        line[2]='P';
        line[3]=tohexchar(mag);
        line[4]=tohexchar(tens);
        line[5]=tohexchar(units);

        active = current_page == desired_page;
    }

    if(!active)
        return 0;

    if(mag != (current_page/100))
        return 0;

    // Strip parity
    for(int i=2; i<sizeof(line); i++)
        line[i] &= 0x7f;

    debug("%d: %-40.40s\n", row, line+2);
    return 0;
}

void PacketParser::set_page(int page)
{
    desired_page = page;
    current_page = -1;
}
