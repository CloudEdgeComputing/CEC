#pragma once

#include <pthread.h>
#include <iostream>
#include "Functions.h"

class QUEUE;
class Task;

// operator는 thread이다.
class OPERATOR
{
private:
    pthread_t tid;
    QUEUE *inqueue, *outqueue;
    Task* parent;
    FUNC pfunc;
    // 현재 operator가 처리중인가?
    bool isUse;
    
    // lock을 위한 lock primitive & condition
    pthread_cond_t g_condition;
    pthread_mutex_t g_mutex;
public:
    
    // 오퍼레이터를 생성한다. 단, 인자로 실제 실행 함수가 주어져야 한다.
    OPERATOR(Task* parent, FUNC pfunc);
    // 오퍼레이터 인풋 큐를 지정한다.
    void setinq(QUEUE* inq);
    // 오퍼레이터 결과 큐를 지정한다.
    void setoutq(QUEUE* outq);
    // 오퍼레이터 인풋 큐를 받는다.
    QUEUE* getinq();
    // 오퍼레이터 결과 큐를 받는다.
    QUEUE* getouq();
    // 오퍼레이터가 실행중인지 묻는다.
    bool getinUse();
    // 오퍼레이터 실행 쓰레드를 만든다.
    void create();
    // 오퍼레이터 쓰레드를 깨운다.
    void wakeup();
    // 오퍼레이터 쓰레드를 재운다./
    void sleep();
    // 오퍼레이터의 컨디션 변수를 가져온다.
    pthread_cond_t* getcondition();
    // 오퍼레이터의 락 변수를 가져온다.
    pthread_mutex_t* getmutex();
    // 오퍼레이터가 실행할 함수 포인터를 가져온다.
    FUNC getpfunc();
    // 오퍼레이터의 부모 태스크 entity를 가져온다.
    Task* getParenttask();
};

static void* wrapper_thread(void* arg);