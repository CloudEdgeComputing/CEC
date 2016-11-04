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
typedef boost::lockfree::queue<TUPLE*> lockfreeq;

struct CLIENT
{
    pthread_t send_tid, recv_tid;
    int fd;
    struct sockaddr_storage* sockaddr;
};

struct CONNDATA
{
    PIPE* inpipe;
    list<PIPE*>* outpipelist;
    PIPE* dispatchpipe;
    int fd;
    void* thispointer;
};