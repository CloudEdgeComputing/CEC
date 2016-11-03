#include "Executor.h"
#include "Connection.h"
#include "Task.h"
#include "Queue.h"
#include "Data.h"
#include "ExecutorManager.h"
#include "Debug.h"

Executor::Executor()
{
    this->outqlist = new list<QUEUE*>;
}

void Executor::executorStart()
{
    printf("For an executor, registered task: %d\n", this->tasks.size());
    for ( auto iter = this->tasks.begin(); iter != this->tasks.end(); ++iter )
    {
        Task* task = *iter;
        printf("task %p is started!\n", task);
        task->SchedulingStart();
    }
}

Connection* Executor::getConnection()
{
    return this->conn;
}

void Executor::setConnection ( Connection* conn )
{
    this->conn = conn;
}

void Executor::setinq ( QUEUE* inq )
{
    this->inq = inq;
    inq->registerforwardDependency(this, TYPE_EXECUTOR);
}

void Executor::setoutq ( QUEUE* outq )
{
    this->outqlist->push_back(outq);
    outq->registerbackDependency(this, TYPE_EXECUTOR);
}

QUEUE* Executor::getinq()
{
    return this->inq;
}

list<QUEUE*>* Executor::getoutqlist()
{
    return this->outqlist;
}

list< Task* > Executor::getTask()
{
    return this->tasks;
}

void Executor::registerTask ( Task* task )
{
    this->tasks.push_back(task);
}

bool Executor::getExecutorState()
{
    return this->state;
}

bool Executor::setExecutorState ( bool state )
{
    this->state = state;
}

void Executor::installReceivedData ( DATA* data )
{
    if(!data->validity())
    {
        printf("Migrated data is not valid!\n");
        return;
    }
    
    // 데이터 분해
    char* p = data->getcontent();
    ushort size = 0;

    debug_packet(data->getcontent(), data->getLen() + 4);
    
    // 오직 한번에 하나의 큐 데이터만 들어옴
    QUEUE* que = NULL;

    if(data->getLen() != 0)
    {
    
        // 단위 데이터를 읽어 설치함
        while(size < data->getLen())
        {
            unsigned int qid = 0;
            memcpy(&qid, p + 8, 4);
            
            // unit data가 스페셜 형식을 가짐
            DATA* unitdata = new DATA(p, true);
                        
            printf("queue id: %d\n", qid);
            //printf("received packet!\n");
            //debug_packet(unitdata->getdata(), unitdata->getLen() + 4);
            
            // qid를 이용해 queue를 얻어옴
            que = executormanager.findQueue(qid);
            
            // queue에 데이터를 푸시함
            que->getQueue()->push(unitdata);
            
            p = p + unitdata->getLen() + 8 + 4;
            size += unitdata->getLen() + 8 + 4;
            //printf("size: %d Len: %d\n", size, data->getLen());
            //printf("unitdata->getLen() + 8: %d\n", unitdata->getLen() + 8 + 4);
        }
        
        que->install_comp_signal();
    }
}

void Executor::printtasks()
{
    for(auto iter = this->tasks.begin(); iter != this->tasks.end(); ++iter)
    {
        Task* task = *iter;
        printf("task: %p\n", task);
    }
    printf("end!\n");
}