#include "TUPLE.h"

/*
 * Packet protocol
 * 0xAA + size (2byte) + type (1byte)
 * nLen은 전체 패킷사이즈
 */

TUPLE::TUPLE ( char* data, ushort nLen, ushort seq, int fd )
{
    this->data = data;
    this->nLen = nLen;
    this->seq = seq;
    this->owner_fd = fd;
    this->content = this->data + 4;
    this->pointer = 0;
}

TUPLE::TUPLE ( ushort nLen, int fd, int type )
{
    this->data = new char [nLen + 4];
    this->data[0] = 0xAA;
    memcpy ( &this->data[1], &nLen, 2 );
    this->data[3] = type;
    this->owner_fd = fd;
    this->pointer = 0;
    this->content = this->data + 4;
}

TUPLE::TUPLE ( char* packet, bool isSpecial )
{
    if ( isSpecial == true )
    {
        // packet + 0 == 프로토콜
        // packet + 4 == ip
        // packet + 8 = port
        // packet + 12 == queue id;
        // packet + 16 == real contents
        
        memcpy ( &this->nLen, &packet[1], 2 );
        this->nLen -= 8; // 현재 컨텐츠에 ip와 큐아이디가 섞여있으므로 빼려고 한다.
        this->data = new char[nLen + 4];
        
        // 0xAA
        this->data[0] = packet[0];
        // Size
        memcpy(&this->data[1], &this->nLen, 2);
        // type
        this->data[3] = packet[3];
        
        memcpy(&this->data[4], &packet[12], this->nLen);
        
        // fd는 쓰레기값.. 변환이 필요함
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

bool TUPLE::validity()
{
    if ( ( unsigned char ) this->data[0] != 0xaa )
    {
        return false;
    }

    return true;
}

uchar TUPLE::gettype()
{
    return this->data[3];
}

char* TUPLE::getcontent()
{
    return this->content;
}

int TUPLE::getfd()
{
    return this->owner_fd;
}

char* TUPLE::getdata()
{
    return this->data;
}

ushort TUPLE::getLen()
{
    return this->nLen;
}

void TUPLE::setpipeid ( uint pipeid )
{
    this->pipeid = pipeid;
}

uint TUPLE::getpipeid()
{
    return this->pipeid;
}

void TUPLE::setfd ( int fd )
{
    this->owner_fd = fd;
}

int TUPLE::getInt()
{
    int result = 0;
    memcpy ( &result, &this->content[pointer], 4 );
    pointer += 4;
    return result;
}

short int TUPLE::getShort()
{
    short result = 0;
    memcpy ( &result, &this->content[pointer], 2 );
    pointer += 2;
    return result;
}

char TUPLE::getChar()
{
    char result = 0;
    result = this->content[pointer];
    pointer += 1;
    return result;
}

void TUPLE::getString ( char* str, int nSize )
{
    memcpy ( str, &this->content[pointer], nSize );
    pointer += nSize;
}

void TUPLE::push ( void* data, ushort size )
{
    memcpy ( &this->content[pointer], data, size );
    pointer += size;
}

void TUPLE::sealing()
{
    this->nLen = pointer;
    this->pointer = 0;
}
