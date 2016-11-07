#pragma once 

#include "CELL.h"
#include <list>
#include <sys/socket.h>  
#include <netinet/in.h>  
#include <netdb.h>  
#include <arpa/inet.h>  
#include <string.h>
#include <pthread.h>
#include <deque>

class PIPE;
class WORKER;
class STREAMFACTORY;
class CELL;
class PACKET;

using namespace std;

class DESTCELL : public CELL 
{
private:
public:
    // outpipe를 가진 srccell을 만든다. 
    DESTCELL( PIPE* inpipe, STREAMFACTORY* parent );
    // outpipe를 가진 srccell을 만든다. 
    DESTCELL( list< PIPE* >* inpipelist, STREAMFACTORY* parent );
    // Virtual, 워커를 만든다.. 지만 사실 센더를 만듦
    virtual void makeWorker();
    // 워커를 실행시킨다. 실제로는 내부 센더 생성
    virtual void* scheduling(void* arg);
    // 소스 브로커 래퍼 함수
    static void* DESTbroker_start_wrapper(void* context);
    // 소스 브로커 인터널 함수
    void* DESTbroker_start_internal(void* context);
};
