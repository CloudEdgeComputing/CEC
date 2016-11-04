#pragma once
#include "Type.h"
#include <string.h>
#include <netinet/in.h>

typedef unsigned short ushort;
typedef unsigned char uchar;

class TUPLE
{
private:
    char* data;
    char* content;
    ushort nLen;
    ushort seq;
    int owner_fd;
    uint pointer;
    uint pipeid;

public:
    TUPLE(char* data, ushort nLen, ushort seq, int fd);
    // Don't use getInt, string, short, char methods with TUPLE(ushort nLen, int fd);
    TUPLE(ushort nLen, int fd, int type);
    // packet을 데이터로 변환. 다만, isSpecial이 true면 packet에 fd와 content가 포함되어있는것으로 가정하여 그 부분은 데이터 컨텐츠에서 뺀다.
    TUPLE ( char* packet, bool isSpecial );
    
    
    bool validity();
    uchar gettype();
    char* getcontent();
    char* getdata();
    ushort getLen();
    // 보내온 fd을 가져온다.
    int getfd();
    // 보내온 fd를 세팅한다.
    void setfd(int fd);
    // 파이프 아이디를 세팅한다.
    void setpipeid(uint pipeid);
    // 파이프 아이디를 받아온다.
    uint getpipeid();
    
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