#include "Task.h"
#include "Operator.h"
#include "Queue.h"
#include "Executor.h"

// Task는 inq 링크, outq 링크, 스케줄러, 오퍼레이터풀로 이루어져 있다.
Task::Task ( QUEUE* inq, QUEUE* outq, ushort count, FUNC func, Executor* parent )
{
    this->inq = inq;
    this->outq = outq;
    this->count = count;
    this->func = func;
    this->parent = parent;
    this->isrun = false;
    this->isEnd = false;
}

void Task::MakeOperators()
{
    this->isrun = false;
    // Operator를 count만큼 만든다.
    for ( int i = 0; i < count; i++ )
    {
        OPERATOR* op = new OPERATOR ( this, this->func );
        op->setinq ( this->inq );
        op->setoutq ( this->outq );
        op->setinUse ( this->isrun );
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
    this->isrun = true;
    pthread_create ( &this->scheduling_tid, NULL, &Task::scheduling_wrapper, this );
}

void* Task::Scheduling ( void* arg )
{   
    while ( 1 )
    {
        // 태스크 스케줄링 종료 요청이 있으면 종료한다.
        if ( this->isEnd )
        {
            exit ( 0 );
        }

        // 태스크 스케줄링 블럭 요청이 있으면 블럭한다.
        if ( !this->isrun )
        {
            printf("Operator scheduler blocked!\n");
            pthread_cond_wait ( &this->g_condition, &this->g_mutex );
            printf("Operator scheduler released!\n");
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
                if ( !op->getinUse() )
                {
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

bool Task::getTaskState()
{
    return this->isrun;
}

bool Task::setTaskState(bool state)
{
    this->isrun = state;
}

QUEUE* Task::getoutq()
{
    return this->outq;
}