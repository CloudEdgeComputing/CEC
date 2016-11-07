#pragma once

#include <stdlib.h>
#include <list>
#include <functional>
#include <pthread.h>
#include "Functions.h"

class PIPE;
class WORKER;
class STREAMFACTORY;

using namespace std;

#define SISO    0
#define MISO    1
#define SO      2
#define SI      3

class CELL
{
    
protected:
    list<PIPE*>* inpipelist;
    list<PIPE*>* outpipelist;
    ushort count;
    STREAMFACTORY* parent;
    bool isrun;
    bool isEnd;
    pthread_t scheduling_tid;
    list<WORKER*> workers;
    ushort XIXO;
    
    // 스케줄링 쓰레드 시그널 컨디션 변수
    pthread_cond_t condition;
    // 스케줄링 쓰레드 시그널 컨디션 변수의 락
    pthread_mutex_t mutex;
    
public:
    CELL( PIPE* inpipe, PIPE* outpipe, ushort count, STREAMFACTORY* parent, uint XIXO );
    CELL( PIPE* inoutpipe, ushort count, STREAMFACTORY* parent, uint XIXO );
    CELL( PIPE* inpipe, list<PIPE*>* outpipelist, ushort count, STREAMFACTORY* parent, uint XIXO );
    CELL( list<PIPE*>* outpipelist, ushort count, STREAMFACTORY* parent, uint XIXO );
    CELL( list<PIPE*>* inpipelist, list<PIPE*>* outpipelist, ushort count, STREAMFACTORY* parent, uint XIXO );
    
    // 워커를 만든다. 워커를 만들 때, 어떤 일을 할지 넣어야 하므로 순수 가상 함수
    virtual void makeWorker() = 0;
    
    // 워커를 스케줄링 한다.
    void schedulingStart();
    
    // 워커 스케줄링 내부 함수, 스케줄링은 CELL마다 다르게 작동하므로 순수 가상 함수
    virtual void* scheduling(void* arg) = 0;
    
    // 워커 스케쥴러 래퍼 함수
    static void* scheduling_wrapper(void* context);
    
    // BASICCELL안의 하나 이상의 워커들이 러닝중인가?
    bool getWORKERState();
    
    // outpipe를 가져온다.
    list<PIPE*>* getoutpipelist();
    
    // 워커 스케줄러 쓰레드를 블럭한러다.
    void schedulerSleep();
    
    // 워커 스케줄러 쓰레드를 깨운다.
    void schedulerWakeup();
    
    // 현재 워커 스케줄러가 러닝 상태인가?
    bool getCELLState();
    
    // 현재 입출력 방식은 무엇인가?
    ushort getXIXO();
    
};
