#pragma once

#include <stdlib.h>
#include <list>
#include <functional>
#include <pthread.h>
#include "Functions.h"

class QUEUE;
class OPERATOR;
class Executor;

using namespace std;

class Task
{
private:
    QUEUE* inq;
    list<QUEUE*>* outqlist;
    ushort count;
    FUNC func;
    Executor* parent;
    std::list<OPERATOR*> ops;
    bool isrun;
    bool isEnd;
    pthread_t scheduling_tid;
    
    // 스케줄링 쓰레드 시그널 컨디션 변수
    pthread_cond_t g_condition;
    // 스케줄링 쓰레드 시그널 컨디션 변수의 락
    pthread_mutex_t g_mutex;
    
public:
    // inq, outq를 갖는 taskpool을 만든다.
    Task( QUEUE* inq, QUEUE* outq, ushort count, FUNC func, Executor* parent );
    Task( QUEUE* inq, list<QUEUE*>* outq, ushort count, FUNC func, Executor* parent );
    // 오퍼레이터를 만든다.
    void MakeOperators();
    // 오퍼레이터를 스케줄링 한다.
    void SchedulingStart();
    // 오퍼레이터 스케줄링 내부 함수
    void* Scheduling(void* arg);
    // 스케쥴링 래퍼 함수
    static void* scheduling_wrapper(void* context);
    // Task안의 하나 이상의 OP들이 러닝중인가?
    bool getOPsState();
    // inq를 가져온다.
    QUEUE* getinq();
    // outq를 가져온다.
    list<QUEUE*>* getoutqlist();
    // 태스크 스케줄러 쓰레드를 블럭한러다.
    void SchedulerSleep();
    // 태스크 스케줄러 쓰레드를 깨운다.
    void SchedulerWakeup();
    // 현재 태스크 스케줄러가 블럭 상태인가?
    bool getTaskState();
};