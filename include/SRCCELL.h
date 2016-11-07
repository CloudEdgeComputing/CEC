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

class SRCCELL : public CELL 
{
private:
    // 어플리케이션 개발자가 지정할 수 있는 함수
    //FUNC func;
    int _broker_sock;
    sockaddr_in _broker_addr;
    int recvport;
    list<struct CLIENT*> devices;
public:
    // outpipe를 가진 srccell을 만든다. 
    SRCCELL( int recvport, PIPE* outpipe, STREAMFACTORY* parent );
    // outpipe를 가진 srccell을 만든다. 
    SRCCELL( int recvport, list< PIPE* >* outpipelist, STREAMFACTORY* parent );
    // Virtual, 워커를 만든다.. 지만 사실 리시버를 만듦
    virtual void makeWorker();
    // 워커를 실행시킨다. 실제로는 내부 리시버 생성
    virtual void* scheduling(void* arg);
    // func를 불러온다.
    //FUNC getfunc();
    // 소스 브로커 래퍼 함수
    static void* SRCbroker_start_wrapper(void* context);
    // 소스 브로커 인터널 함수
    void* SRCbroker_start_internal(void* context);
    // 소스 리시버 래퍼 함수스
    static void* SRCrecv_start_wrapper(void* context);
    // 소스 리시버 인터널 함수
    void* SRCrecv_start_internal(void* context);
    // 패킷 어셈블러
    void pushPacket(char* input, int size, std::deque< PACKET* >* packetque, unsigned char* seq);
    // 패킷 어셈된것 받기
    char* getPacketassembled(int* size, std::deque< PACKET* >* packetque);
    // 디바이스를 가입시킨다.
    void registerDevice(CLIENT* client);
};
