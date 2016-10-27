#pragma once

#include <stdlib.h>
#include <list>
#include <functional>
#include <pthread.h>
#include "Functions.h"

class QUEUE;
class OPERATOR;
class Executor;

class Task
{
private:
    QUEUE* inq, *outq;
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
    // 오퍼레이터를 만든다.
    void MakeOperators();
    // 오퍼레이터를 스케줄링 한다.
    void SchedulingStart();
    // 오퍼레이터 스케줄링 내부 함수
    void* Scheduling(void* arg);
    // 스케쥴링 래퍼 함수
    static void* scheduling_wrapper(void* context);
    // Task가 running중인가?
    bool getTaskState();
    // Task의 상태를 변경한다.
    bool setTaskState(bool state);
    // outq를 가져온다.
    QUEUE* getoutq();
};