#include "ExecutorManager.h"
#include "Task.h"
#include "Queue.h"
#include "Executor.h"
#include "Connection.h"
#include "Functions.h"
#include "Data.h"
#include "Migration.h"

void ExecutorManager::createExecutor(ushort recvport)
{
    printf("Executor instance creation...\n");
    // 익스큐터 인스턴스 생성 및 등록
    Executor* executor = new Executor;
    this->execs.push_back ( executor );

    printf("Executor inputqueue and outputqueue creation...\n");
    // 익스큐터 전체의 인풋 아웃풋 큐 생성
    executor->setinq( this->makeQueue( UNDEFINED, TYPE_CONNECTION ) );
    executor->setoutq( this->makeQueue( executor, TYPE_EXECUTOR ) );
    
    printf("Executor connection setting...\n");
    
    // 익스큐터에 연결할 커넥션 생성
    executor->setConnection(new Connection(recvport, executor));
    executor->getConnection()->serverStart(executor->getinq(), executor->getoutq());

    executor->getinq()->registerbackDependency(executor->getConnection(), TYPE_CONNECTION);
    executor->getinq()->registerforwardDependency(executor, TYPE_EXECUTOR);
    executor->getoutq()->registerforwardDependency(executor->getConnection(), TYPE_CONNECTION);
    
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
    Task* task1 = new Task ( executor->getinq(), task1que, 3, func1, executor );
    Task* task2 = new Task ( task1que, executor->getoutq(), 3, func2, executor );
    
    printf("registered task1 %p, %p\n", task1, task2);
    
    task1que->registerbackDependency(task1, TYPE_TASK);
    task1que->registerforwardDependency(task2, TYPE_TASK);
    
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