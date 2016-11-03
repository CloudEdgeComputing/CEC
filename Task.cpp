#include "Task.h"
#include "Operator.h"
#include "Queue.h"
#include "Executor.h"

// Task는 inq 링크, outq 링크, 스케줄러, 오퍼레이터풀로 이루어져 있다.
Task::Task ( QUEUE* inq, QUEUE* outq, ushort count, FUNC func, Executor* parent )
{
    this->outqlist = new list<QUEUE*>;
    
    if(inq != NULL)
    {
        this->inq = inq;
        inq->registerforwardDependency(this, TYPE_TASK);
    }
    
    if(outq != NULL)
    {
        this->outqlist->push_back(outq);
        outq->registerbackDependency(this, TYPE_TASK);
    }
    
    this->count = count;
    this->func = func;
    this->parent = parent;
    this->isrun = true;
    this->isEnd = false;
}

Task::Task ( QUEUE* inq, list<QUEUE*>* outqlist, ushort count, FUNC func, Executor* parent )
{
    this->outqlist = new list<QUEUE*>;
    
    if(inq != NULL)
    {
        this->inq = inq;
        inq->registerforwardDependency(this, TYPE_TASK);
    }
    
    for(auto iter = outqlist->begin(); iter != outqlist->end(); ++iter)
    {
        QUEUE* que = *iter;
        this->outqlist->push_back(que);
        que->registerbackDependency(this, TYPE_TASK);
    }
    
    this->count = count;
    this->func = func;
    this->parent = parent;
    this->isrun = true;
    this->isEnd = false;
}

void Task::MakeOperators()
{
    // Operator를 count만큼 만든다.
    for ( int i = 0; i < count; i++ )
    {
        OPERATOR* op = new OPERATOR ( this, this->func );
        ops.push_back ( op );
    }

    // Task를 false시키고 operator를 런 시키면 모든 task는 생성 뒤, 블럭 상태가 된다.
    for ( auto iter = ops.begin(); iter != ops.end(); ++iter )
    {
        OPERATOR* op = *iter;
        op->create();
    }
}

void Task::SchedulingStart()
{
    pthread_create ( &this->scheduling_tid, NULL, &Task::scheduling_wrapper, this );
}

void* Task::Scheduling ( void* arg )
{   
    while ( 1 )
    {
        // 태스크 스케줄링 종료 요청이 있으면 종료한다.
        if ( this->isEnd )
        {
            printf("exit in task scheduling!\n");
            exit ( 0 );
        }

        // 태스크 스케줄링 블럭 요청이 있으면 블럭한다.
        if ( !this->isrun )
        {
            printf("Task scheduler blocked!\n");
            pthread_cond_wait ( &this->g_condition, &this->g_mutex );
            printf("Task scheduler released!\n");
        }
        

        /*
         * 오퍼레이터의 블럭킹 해제 조건
         * inq에 데이터가 있으며 오퍼레이터중에 실행중이 아닌것이 있으면 깨워서 실행한다..
         */
        lockfreeq* inq = this->inq->getQueue();
        if ( !inq->empty() )
        {
            auto iter = this->ops.begin();
            for ( ; iter != this->ops.end(); ++iter )
            {
                OPERATOR* op = *iter;
                //printf("op->getinUse: %d\n", op->getinUse());
                if ( !op->getinUse() )
                {
                    //printf("woke up!\n");
                    op->wakeup();
                }
            }
        }
    }
}

void* Task::scheduling_wrapper ( void* context )
{
    return ((Task*)context)->Scheduling(context);
}

bool Task::getOPsState()
{
    bool checker = false;
    int cnt = 0;
    // 모든 오퍼레이터 검색하여 작동하고 있는 오퍼레이터가 없는 경우 false, 하나라도 있는 경우 true
    for(auto iter = ops.begin(); iter != ops.end(); ++iter)
    {
        OPERATOR* op = *iter;
        checker |= op->getinUse();
        if(op->getinUse() == true)
            cnt++;
    }
    
    //printf("inUse operator: %d\n", cnt);
    
    return checker;
}

QUEUE* Task::getinq()
{
    return this->inq;
}

list<QUEUE*>* Task::getoutqlist()
{
    return this->outqlist;
}

void Task::SchedulerSleep()
{
    pthread_mutex_lock(&this->g_mutex);
    this->isrun = false;
    pthread_mutex_unlock(&this->g_mutex);
}

void Task::SchedulerWakeup()
{
    pthread_mutex_lock(&this->g_mutex);
    this->isrun = true;
    pthread_cond_signal(&this->g_condition);
    //printf("Op wake up...\n");
    pthread_mutex_unlock(&this->g_mutex);
}

bool Task::getTaskState()
{
    return this->isrun;
}