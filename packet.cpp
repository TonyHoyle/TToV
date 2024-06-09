#include <unistd.h>
#include <string.h>
#include <stdint.h>
#include "packet.h"
#include "debug.h"

uint8_t deham(uint8_t value)
{
    return ((value&0x80)>>4)|((value&0x20)>>3)|((value&0x08)>>2)|((value&0x02)>>1);
}

uint8_t deham2(uint8_t *values)
{
    return (deham(values[1]) << 4) | deham(values[0]);
}

char tohexchar(uint8_t val)
{
    return (val>9)?val+'0'+7:val+'0';
}

int parse_packet(int input_channel, int output_channel)
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
        memset(line+2, 32, 8);
        line[2]='P';
        line[3]=tohexchar(mag);
        line[4]=tohexchar(tens);
        line[5]=tohexchar(units);
    }

    // Strip parity
    for(int i=2; i<sizeof(line); i++)
        line[i] &= 0x7f;

    if(mag!=1) return 0;
    debug("%d,%d: %-40.40s\n", mag,row, line+2);
    return 0;
}
