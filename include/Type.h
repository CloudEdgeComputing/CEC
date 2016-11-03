#pragma once

#include <boost/lockfree/queue.hpp>
#include <sys/socket.h> 
#include <list>

using namespace boost;
using namespace std;

class QUEUE;
class DATA;

typedef unsigned long DWORD;
typedef unsigned short WORD;
typedef unsigned char BYTE;
typedef unsigned short ushort;
typedef unsigned char uchar;
typedef boost::lockfree::queue<DATA*> lockfreeq;

struct CLIENT
{
    pthread_t send_tid, recv_tid;
    int fd;
    struct sockaddr_storage* sockaddr;
};

struct CONNDATA
{
    QUEUE* inq;
    list<QUEUE*>* outqlist;
    QUEUE* dispatchq;
    int fd;
    void* thispointer;
};