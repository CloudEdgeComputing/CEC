#include "PACKET.h"
#include <string.h>

PACKET::PACKET ( char* in_array, unsigned int size, unsigned char seq )
{
    this->bytearray = new char[size];
    memcpy(bytearray, in_array, size);
    this->size = size;
    this->seq = seq;
}

char* PACKET::getArray()
{
    return this->bytearray;
}

unsigned int PACKET::getSize()
{
    return this->size;
}

unsigned char PACKET::getSeq()
{
    return this->seq;
}

void PACKET::setArray ( char* arr )
{
    this->bytearray = arr;
}

void PACKET::setSize ( unsigned int size )
{
    this->size = size;
}