#include "ExecutorManager.h"
#include "Task.h"
#include "Queue.h"
#include "Executor.h"
#include "Connection.h"
#include "Functions.h"
#include "Data.h"
#include "Migration.h"

// Task, Connection에 outq는 2개 이상이 붙을 수 있고, inq는 항상 하나다.
void ExecutorManager::createExecutor(ushort recvport)
{
    // 익스큐터 인스턴스 생성 및 등록
    printf("Executor instance creation...\n");
    Executor* executor = new Executor;
    this->execs.push_back ( executor );

    // 익스큐터 전체의 인풋 아웃풋 큐 생성
    printf("Executor inputqueue and outputqueue creation...\n");
    executor->setinq( this->makeQueue( UNDEFINED, TYPE_CONNECTION ) );
    printf("global id for inq: %d\n", executor->getinq()->getid());
    executor->setoutq( this->makeQueue( executor, TYPE_EXECUTOR ) );
    
    auto outqlist = executor->getoutqlist();
    for(auto iter = outqlist->begin(); iter != outqlist->end(); ++iter)
    {
        QUEUE* que = *iter;
        printf("global id for outq: %d\n", que->getid());
    }
    
    // 익스큐터에 연결할 커넥션 생성
    printf("Executor connection setting...\n");
    executor->setConnection(new Connection(recvport, executor));
    executor->getConnection()->serverStart(executor->getinq(), executor->getoutqlist());
    
    printf("Task creation...\n");
    /*
     * 들어오는 함수는 인자로 DATA* 를 써야 하며, DATA*를 쓴 후에는 꼭 메모리 해제가 필요하다.
     */
    // Script 해석부
    FUNC func1 = task_1;
    FUNC func2 = task_2;
    printf("registred func1 %p, %p\n", func1, func2);

    // Task 생성
    QUEUE* task1que = this->makeQueue( UNDEFINED, TYPE_TASK );
    printf("global id for task1que : %d\n", task1que->getid());
    Task* task1 = new Task ( executor->getinq(), task1que, 2, func1, executor );
    Task* task2 = new Task ( task1que, executor->getoutqlist(), 2, func2, executor );
    printf("registered task1 %p, %p\n", task1, task2);
    
    printf("Operation creation for each task...\n");
    // Task 내부 오퍼레이터 생성
    task1->MakeOperators();
    task2->MakeOperators();

    printf("Task registering to executor instance...\n");
    // 생성된 task를 executor에 등록시킴
    executor->registerTask(task1);
    executor->registerTask(task2);
    
    // 스크립트 해석 끝
}

void ExecutorManager::executorRunAll()
{
    printf("registered excutors: %d\n", this->execs.size());
    for ( auto iter = this->execs.begin(); iter != this->execs.end(); ++iter )
    {
        Executor* executor = *iter;
        executor->executorStart();
    }
}

Executor* ExecutorManager::getExecutorbyId ( int id )
{
    int cnt = 0;
    
    for (auto iter = this->execs.begin(); iter != this->execs.end(); ++iter)
    {
        if(cnt == id)
            return *iter;
        else
            cnt++;
    }
    printf("Unrecognized executor id!\n");
    exit(0);
    return NULL;
}

QUEUE* ExecutorManager::makeQueue(void* powner, int type)
{
    int id = this->new_gid++;
    QUEUE* que = new QUEUE ( new lockfreeq(0), powner, type, id );
    ques.push_back( que );
    
    return que;
}

QUEUE* ExecutorManager::findQueue ( unsigned int id )
{
    for(auto iter = ques.begin(); iter != ques.end(); ++iter)
    {
        QUEUE* que = *iter;
        if(id == que->getid())
            return que;
    }
    
    printf("Queue is not found!\n");
    exit(0);
    return NULL;
}

void ExecutorManager::setMigrationModule ( Migration* mig )
{
    this->migration = mig;
}

Migration* ExecutorManager::getMigrationModule()
{
    return this->migration;
}

void ExecutorManager::startMigration()
{
    this->migration->startMigration(0, *this->execs.begin());
}