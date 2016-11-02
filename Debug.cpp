#include "Debug.h"
#include <iostream>

using namespace std;

void debug_packet(char* packet, unsigned int len)
{
    printf("packet: ");
    for(int i = 0; i < len; i++)
    {
        printf("0x%x ", (unsigned char)packet[i]);
    }
    printf("\n");
}