#pragma once

#include <boost/lockfree/queue.hpp>
#include <sys/socket.h> 
#include <list>

using namespace boost;
using namespace std;

class PIPE;
class TUPLE;

typedef unsigned long DWORD;
typedef unsigned short WORD;
typedef unsigned char BYTE;
typedef unsigned short ushort;
typedef unsigned char uchar;
typedef list<TUPLE*> quetype;

struct CLIENT
{
    int uuid;
    int fd;
    struct sockaddr* var_sockaddr;
};

struct CONNDATA
{
    list<PIPE*>* inpipelist;
    list<PIPE*>* outpipelist;
    int fd;
    CLIENT* client;
    void* thispointer;
};