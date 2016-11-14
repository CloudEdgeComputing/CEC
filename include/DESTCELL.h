#pragma once 

/**
 *  @file DESTCELL.h
 *  @brief destcell을 정의해놓은 헤더
 */

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

/**
 * @brief destcell을 정의해놓은 클래스. factory에서 나가는 데이터의 종말 지점이다.
 * @details destcell을 정의해놓은 클래스. factory에서 나가는 데이터의 종말 지점이다. cell의 형태를 상속하며 SI의 형태를 갖는다. 현재는 종말지점은 소스셀에서 연결된 디바이스 중 하나로만 한정한다. 만약 여러개의 pipe가 input으로 연결되면 라운드 로빈으로 처리한다. 이 안에서의 워커는 개념상 1만 존재한다.
 * @author ysmoon
 * @date 2016-11-08
 * @version 0.0.1
 * @todo DB로 보내기, src cell에 연결되지 않은 device도 연결을 받아 보내기
 */

class DESTCELL : public CELL 
{
private:
public:
    /**
     *  @brief destcell을 만드는 생성자.
     *  @details destcell을 만드는 생성자. 단일 inpipe로 연결되어 있다.
     *  @param inpipe destcell로 데이터를 넘겨주기 위한 pipe
     *  @param parent cell이 속한 factory를 명시한다.
     */
    DESTCELL( PIPE* inpipe, STREAMFACTORY* parent );
    /**
     *  @brief destcell을 만드는 생성자.
     *  @details destcell을 만드는 생성자. 단일 inpipe로 연결되어 있다.
     *  @param inpipelist destcell로 데이터를 넘겨주기 위한 pipe의 리스트. 깊은 복사를 하며 사용 이후 inpipelist가 필요가 없으면 해제하는것이 좋다.
     *  @param parent cell이 속한 factory를 명시한다.
     */
    DESTCELL( list< PIPE* >* inpipelist, STREAMFACTORY* parent );
    /**
     *  @brief dummy
     *  @details dummy
     */
    virtual void makeWorker();
    /**
     *  @brief scheduling이지만, worker가 내부적으로 1개이므로 worker의 역할을 할 수 있도록 쓰레드를 생성한다.
     *  @details scheduling이지만, worker가 내부적으로 1개이므로 worker의 역할을 할 수 있도록 쓰레드를 생성한다. 이 쓰레드에서 최종적으로 데이터를 읽고 보낸다.
     *  @param arg (DESTCELL의) this pointer를 넘긴다.
     *  @return void*는 의미 없다.
     */
    virtual void* scheduling(void* arg);
    /**
     *  @brief 센더 스레드를 만들기 위한 래퍼 함수이다.
     *  @details 센더 스레드를 만들기 위한 래퍼 함수이다.
     *  @param context (DESTCELL의) this pointer를 넘긴다.
     *  @return void*는 의미 없다.
     */
    static void* DESTbroker_start_wrapper(void* context);
    /**
     *  @brief 센더 래퍼함수에서 실제로 수행되는 로직이 담긴 함수이다.
     *  @details 센더 래퍼함수에서 실제로 수행되는 로직이 담긴 함수이다. 내부에서는 input pipe들을 라운드 로빈으로 읽어 알맞는 디바이스로 데이터를 전송한다.
     *  @param context (DESTCELL의) this pointer를 넘긴다.
     *  @return void*는 의미 없다.
     */
    void* DESTbroker_start_internal(void* context);
};
