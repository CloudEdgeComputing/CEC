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

/**
 *  @file SRCCELL.h
 *  @brief srccell을 정의해놓은 헤더
 */
class PIPE;
class WORKER;
class STREAMFACTORY;
class CELL;
class PACKET;
class FACTORYBUILDER;

extern FACTORYBUILDER factorymanager;

using namespace std;

/**
 * @brief srccell을 정의해놓은 클래스. factory로 들어오는 데이터의 시작 지점이다. cell의 형태를 상속하며 SO의 형태를 갖는다.
 * @details srccell을 정의해놓은 클래스. cell의 형태를 상속하며 SO의 형태를 갖는다. 현재는 하나의 소스셀은 서버를 만들고 여기에 디바이스가 접속할 수 있게만 되어 있다.
 * @author ysmoon
 * @date 2016-11-08
 * @version 0.0.1
 */
class SRCCELL : public CELL 
{
private:
    int _broker_sock;
    sockaddr_in _broker_addr;
    int recvport;
    list<struct CLIENT*> devices;
public:
    /**
     *  @brief srccell을 만드는 생성자.
     *  @details dsetcell을 만드는 생성자. 워커는 srccell에 연결된 디바이스당 하나가 생긴다. 스케줄링 개념은 없으며 일반적 서버-클라이언트 모델에 서버에서의 리시버 스레드 형태를 워커로써 갖는다.
     *  @param recvport 소스셀에 접속할 때의 포트를 정의한다.
     *  @param outpipe cell에 대한 단일 output pipe이다.
     *  @param parent cell이 속한 factory를 명시한다.
     */
    SRCCELL( int recvport, PIPE* outpipe, STREAMFACTORY* parent );
    /**
     *  @brief srccell을 만드는 생성자.
     *  @details destcell을 만드는 생성자. 워커는 srccell에 연결된 디바이스당 하나가 생긴다. 스케줄링 개념은 없으며 일반적 서버-클라이언트 모델에 서버에서의 리시버 스레드 형태를 워커로써 갖는다.
     *  @param recvport 소스셀에 접속할 때의 포트를 정의한다.
     *  @param outpipelist cell에 대한 output pipe list를 넘겨준다. 포인터이며 안에서는 깊은복사가 일어난다. 파라미터로 들어온 포인터는 필요없으면 해제하는것이 좋다.
     *  @param parent cell이 속한 factory를 명시한다.
     */
    SRCCELL( int recvport, list< PIPE* >* outpipelist, STREAMFACTORY* parent );
    /**
     *  @brief srccell의 워커(리시버)를 만들기 위해 설정한다.
     *  @details srccell의 워커(리시버)를 만들기 위해 설정한다.
     *  @return void 의미 없음
     */
    virtual void makeWorker();
    /**
     *  @brief 이름은 스케줄링이지만 리시버 브로커 쓰레드를 하나 만드는 함수이다. 브로커 쓰레드는 결국 accept를 부르며 클라이언트 연결 대기상태로 만든다.
     *  @details 이름은 스케줄링이지만 리시버 브로커 쓰레드를 하나 만드는 함수이다. 브로커 쓰레드는 결국 accept를 부르며 클라이언트 연결 대기상태로 만든다.
     *  @return void 의미 없음
     */
    virtual void* scheduling(void* arg);
    /**
     *  @brief 브로커 쓰레드에서 실제로 실행되는 래퍼 함수.
     *  @details 브로커 쓰레드에서 실제로 실행되는 래퍼 함수. 존재 의의는 this 포인터를 넘기고 사용하기 위한 trick함수이다.
     *  @param arg (SRCELL의)this pointer가 들어온다.
     *  @return void* 의미 없음
     */
    static void* SRCbroker_start_wrapper(void* context);
    /**
     *  @brief SRCCELL에서 연결 대기를 실제로 수행하는 함수
     *  @details SRCCELL에서 연결 대기를 실제로 수행하는 함수 this pointer를 이용할 수 있다.
     *  @param context (SRCELL의)this pointer가 들어온다.
     *  @return void* 의미 없음
     */
    void* SRCbroker_start_internal(void* context);
    /**
     *  @brief SRCCELL에서 대기상태에서 연결요청이 들어왔을 때 만드는 리시버 쓰레드에 들어가는 함수
     *  @details SRCCELL에서 대기상태에서 연결요청이 들어왔을 때 만드는 리시버 쓰레드에 들어가는 함수. 쓰레드당 디바이스가 1:1로 매핑되어 생성된다.
     *  @param context (SRCELL의)this pointer가 들어온다.
     *  @return void* 의미 없음
     */
    static void* SRCrecv_start_wrapper(void* context);
    /**
     *  @brief 실제 리시버 쓰레드에서 데이터를 받는 함수
     *  @details 실제 리시버 쓰레드에서 데이터를 받는 함수. wrapper 함수에서 부른다.
     *  @param context (SRCELL의)this pointer가 들어온다.
     *  @return void* 의미 없음
     */
    void* SRCrecv_start_internal(void* context);
    /**
     *  @brief 들어온 패킷을 넣는다. 
     *  @details 들어온 패킷을 넣는다. 이 패킷들은 getPacketassembled에서 조립되어 받을 수 있다.
     *  @param input 패킷의 바이트 스트림
     *  @param size 들어온 패킷의 사이즈
     *  @param packetque 들어온 패킷을 저장해놓은 큐
     *  @param seq 패킷의 시퀀스 넘버를 할당하기 위한 공유변수
     */
    void pushPacket(char* input, int size, deque< PACKET* >* packetque, unsigned char* seq);
    /**
     *  @brief 넣은 패킷을 하나의 처리 가능한 패킷으로 돌려 받는다.
     *  @details 넣은 패킷을 하나의 처리 가능한 패킷으로 돌려받는다. 처리 가능한 패킷이란 프로토콜을 완벽히 가지고 있고, 프로토콜만큼의 데이터 사이즈를 가진 패킷을 말한다.
     *  @param size 하나의 처리 가능한 패킷의 전체 사이즈
     *  @param packetque 들어온 패킷을 저장해놓은 큐
     */
    char* getPacketassembled(int* size, deque< PACKET* >* packetque);
    /**
     *  @brief 디바이스와 현재 연결된 정보를 저장한다.
     *  @details 디바이스와 현재 연결된 정보를 저장한다. 이 정보는 나중에 destcell에서 데이터를 보낼 때 사용된다.
     *  @param client 디바이스의 접속 정보이다. 연결 시 필요한 sockaddr과 fd을 가지고 있다.
     */
    void registerDevice(CLIENT* client);
};
