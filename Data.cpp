#include "Data.h"

/*
 * Packet protocol
 * 0xAA + size (2byte) + type (1byte)
 * nLen은 전체 패킷사이즈
 */

DATA::DATA ( char* data, ushort nLen, ushort seq, int fd )
{
    this->data = data;
    this->nLen = nLen;
    this->seq = seq;
    this->owner_fd = fd;
    this->content = this->data + 4;
    this->pointer = 0;
}

DATA::DATA ( ushort nLen, int fd, int type )
{
    this->data = new char [nLen + 4];
    this->data[0] = 0xAA;
    memcpy ( &this->data[1], &nLen, 2 );
    this->data[3] = type;
    this->owner_fd = fd;
    this->pointer = 0;
    this->content = this->data + 4;
}

DATA::DATA ( char* packet, bool isSpecial )
{
    if ( isSpecial == true )
    {
        // packet + 0 == 프로토콜
        // packet + 4 == fd
        // packet + 8 == queue id;
        // packet + 12 == real contents
        
        memcpy ( &this->nLen, &packet[1], 2 );
        this->nLen -= 8; // 현재 컨텐츠에 fd와 큐아이디가 섞여있으므로 빼려고 한다.
        this->data = new char[nLen + 4];
        
        this->data[0] = packet[0];
        memcpy(&this->data[1], &this->nLen, 2);
        this->data[3] = packet[3];
        
        memcpy(&this->data[4], &packet[12], this->nLen);
        
        memcpy ( &this->owner_fd, &packet[4], 4 );
        this->pointer = 0;
        this->content = this->data + 4;
    }
    else
    {
        printf("Not implemented!\n");
        exit(0);
    }
}

bool DATA::validity()
{
    if ( ( unsigned char ) this->data[0] != 0xaa )
    {
        return false;
    }

    if ( this->nLen > 1024 )
    {
        return false;
    }

    return true;
}

uchar DATA::gettype()
{
    return this->data[3];
}

char* DATA::getcontent()
{
    return this->content;
}

int DATA::getfd()
{
    return this->owner_fd;
}

char* DATA::getdata()
{
    return this->data;
}

ushort DATA::getLen()
{
    return this->nLen;
}

void DATA::setfd ( int fd )
{
    this->owner_fd = fd;
}

int DATA::getInt()
{
    int result = 0;
    memcpy ( &result, &this->content[pointer], 4 );
    pointer += 4;
    return result;
}

short int DATA::getShort()
{
    short result = 0;
    memcpy ( &result, &this->content[pointer], 2 );
    pointer += 2;
    return result;
}

char DATA::getChar()
{
    char result = 0;
    result = this->content[pointer];
    pointer += 1;
    return result;
}

void DATA::getString ( char* str, int nSize )
{
    memcpy ( str, &this->content[pointer], nSize );
    pointer += nSize;
}

void DATA::push ( void* data, ushort size )
{
    memcpy ( &this->content[pointer], data, size );
    pointer += size;
}

void DATA::sealing()
{
    this->nLen = pointer;
    this->pointer = 0;
}
