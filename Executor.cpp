#include "Executor.h"
#include "Connection.h"
#include "Task.h"
#include "Queue.h"

void Executor::executorStart()
{
    printf("For an executor, registered task: %d\n", this->tasks.size());
    for ( auto iter = this->tasks.begin(); iter != this->tasks.end(); ++iter )
    {
        Task* task = *iter;
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
}

void Executor::setoutq ( QUEUE* outq )
{
    this->outq = outq;
}

QUEUE* Executor::getinq()
{
    return this->inq;
}

QUEUE* Executor::getoutq()
{
    return this->outq;
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