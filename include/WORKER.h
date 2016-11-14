#pragma once

#include <pthread.h>
#include <iostream>
#include <list>
#include <deque>
#include "Functions.h"

class PIPE;
class CELL;

using namespace std;

// WORKER class is subjected to BASICCELL
class WORKER
{
private:
    pthread_t tid;
    CELL* parentCELL;
    // 현재 operator가 처리중인가?
    bool working;

    // lock을 위한 lock primitive & condition
    pthread_cond_t condition;
    pthread_mutex_t mutex;
    
    // lock을 위한 lock primitive & condition
    pthread_mutex_t MISOmutex;
    // 어떤 uuid를 가진 tuple을 처리하고 있는가?
    list<unsigned int> uuid;
public:

    // 오퍼레이터를 생성한다. 단, 인자로 실제 실행 함수가 주어져야 한다.
    WORKER ( CELL* parent );
    // 오퍼레이터가 실행중인지 묻는다.
    bool isWorking();
    // 오퍼레이터 실행 쓰레드를 만든다.
    void create();
    // 오퍼레이터 쓰레드를 깨운다.
    void wakeup();
    // 오퍼레이터 쓰레드를 재운다./
    void sleep();
    // 오퍼레이터의 부모 태스크 entity를 가져온다.
    CELL* getParentCELL();
    // set uuid
    void setuuid(unsigned int uuid);
    // 특정 uuid를 가지고 있는가?
    bool hasuuid(unsigned int uuid);
    // uuid reset
    void clearuuid();
};

static void* wrapper_thread ( void* arg );
