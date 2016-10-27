#pragma once
#include "Type.h"
#include <string.h>
#include <netinet/in.h>

typedef unsigned short ushort;
typedef unsigned char uchar;

class DATA
{
private:
    char* data;
    char* content;
    ushort nLen;
    ushort seq;
    int owner_fd;
    unsigned int pointer;

public:
    DATA(char* data, ushort nLen, ushort seq, int fd);
    // Don't use getInt, string, short, char methods with DATA(ushort nLen, int fd);
    DATA(ushort nLen, int fd, int type);
    bool validity();
    uchar gettype();
    char* getcontent();
    char* getdata();
    ushort getLen();
    // 보내온 fd을 가져온다.
    int getfd();
    // 보내온 fd를 세팅한다.
    void setfd(int fd);
    
    // Deserialization method
    // get integer
    int getInt();
    // get string
    void getString(char* str, int nSize);
    // get short
    short getShort();
    // get char
    char getChar();
    
    // Data maker method
    // Push data with size
    void push(void* data, ushort size);
    // Data sealing to send
    void sealing();
};