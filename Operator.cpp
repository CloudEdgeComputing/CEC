#include "Operator.h"
#include "Queue.h"
#include "Task.h"
#include "Data.h"

static void debug_packet(char* packet, unsigned int len)
{
    printf("packet: ");
    for(int i = 0; i < len; i++)
    {
        printf("0x%x ", packet[i]);
    }
    printf("\n");
}

OPERATOR::OPERATOR( Task* parent, FUNC pfunc )
{
    this->parent = parent;
    this->pfunc = pfunc;
    this->isUse = false;
    this->g_mutex = PTHREAD_MUTEX_INITIALIZER;
    this->g_condition = PTHREAD_COND_INITIALIZER;
}

void OPERATOR::setinq(QUEUE* inq)
{
    this->inqueue = inq;
}

void OPERATOR::setoutq(QUEUE* outq)
{
    this->outqueue = outq;
}

QUEUE* OPERATOR::getinq()
{
    return this->inqueue;
}

QUEUE* OPERATOR::getouq()
{
    return this->outqueue;
}

void OPERATOR::setinUse ( bool sw )
{
    this->isUse =  sw;
}

bool OPERATOR::getinUse()
{
    return this->isUse;
}

void OPERATOR::create()
{
    // 런 상태가 안되어있으면 wrapper는 self blocking함
    // 런 상태가 되어있으면 만들자마자 돌아감
    pthread_create(&this->tid, NULL, wrapper_thread, (void*)this);
}

void OPERATOR::wakeup()
{
    pthread_mutex_lock(&this->g_mutex);
    this->setinUse(true);
    pthread_cond_signal(&this->g_condition);
    //printf("Op wake up...\n");
    pthread_mutex_unlock(&this->g_mutex);
}

void OPERATOR::sleep()
{
    pthread_mutex_lock(&this->g_mutex);
    this->setinUse(false);
    pthread_cond_wait(this->getcondition(), this->getmutex());
    //printf("Op sleeping...\n");
    pthread_mutex_unlock(&this->g_mutex);
}

// wrapper_thread를 이용하여 오퍼레이터 함수 실행시키기
static void* wrapper_thread(void* arg)
{
    OPERATOR* op = (OPERATOR*)arg;
    boost::lockfree::queue<DATA*>* inq, *outq;
    inq = op->getinq()->getQueue();
    outq = op->getouq()->getQueue();
    
    while(1)
    {
        /* 아래 조건들 중 하나가 만족되면 블럭된다.
         * 1. 읽어올 inq가 empty이다.
         * 2. 러닝 상태이다.
         */
        if(inq->empty())
        {
            op->sleep();
            // 여기에 들어오면 inq에 정보가 있을것이므로 다시 체크
            continue;
        }
        
        DATA* data;
        
        // inq에서 데이터를 꺼낸다.
        if(!inq->pop(data))
        {
            printf("Error occured in wrapper thread pop!\n");
            exit(0);
        }
        
        // Sequence check TODO
        //debug_packet(data->getcontent(), (unsigned int)data->getLen());
        
        // function call using data
        
        typedef DATA* (*FUNC)(DATA*);
        FUNC func = (FUNC)op->getpfunc();
        DATA* output = func(data);
        
        // 오너 소켓디스크립터는 변하지 않았으므로 유지한다.
        output->setfd(data->getfd());
        
        // outq로 결과 데이터를 보낸다.
        if(!outq->push(output))
        {
            printf("Error occured in wrapper thread push!\n");
            exit(0);
        }
        
        printf("Operator completed!\n");
    }
}

pthread_cond_t* OPERATOR::getcondition()
{
    return &this->g_condition;
}

pthread_mutex_t* OPERATOR::getmutex()
{
    return &this->g_mutex;
}

FUNC OPERATOR::getpfunc()
{
    return this->pfunc;
}