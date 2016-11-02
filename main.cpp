#include <stdio.h>
#include "ExecutorManager.h"
#include "Migration.h"
#include <Executor.h>
#include <Task.h>
#include "Data.h"
#include "Queue.h"
#include <Connection.h>

using namespace std;

ExecutorManager executormanager;
  

int main ( int argc, char **argv )
{   
    printf ( "Put 's' for server, 'c' for client\n" );
    char cp;
    scanf("%c", &cp);
    
    if ( cp == 's' )
    {
        Migration Mig(1200, cp);
        executormanager.setMigrationModule(&Mig);
        // 익스큐터 생성 후, 내부에서 인자를 이용하여 커넥션을 생성
        
        executormanager.createExecutor ( 1237 );
        
        // 등록된 모든 익스큐터 런런런
        executormanager.executorRunAll();   
    }
    else
    {
        Migration Mig(1200, cp);
        executormanager.setMigrationModule(&Mig);
        // 익스큐터 생성 후, 내부에서 인자를 이용하여 커넥션을 생성
        executormanager.createExecutor ( 1239 );

        // 등록된 모든 익스큐터 런런런
        executormanager.executorRunAll();
    }
    
    printf("completion of basis\n");
    
    
    // For debug
    while(1)
    {
        scanf("%c", &cp);
        if(cp == 'd')
            executormanager.startMigration();
        else if(cp == 'p')
        {
            Executor* executor = executormanager.getExecutorbyId(0);
            // 모든 Task를 블럭한다.
            auto list = executor->getTask();
            for(auto iter = list.begin(); iter != list.end(); ++iter)
            {
                Task* task = *iter;
                task->SchedulerSleep();
            }
            
            // Conection을 블럭한다.
            Connection* conn = executor->getConnection();
            conn->sleepConnection();
            
            // 데이터를 삽입한다.
            int value = 4;
            DATA* data = new DATA(4, 1, 0);
            data->push(&value, 4);
            data->sealing();
            
            int value2 = 5;
            DATA* data2 = new DATA(4, 1, 0);
            data2->push(&value2, 4);
            data2->sealing();
            
            for(auto iter = list.begin(); iter != list.end(); ++iter)
            {
                Task* task = *iter;
                lockfreeq* que = task->getinq()->getQueue();
                que->push(data);
                que->push(data2);
            }
            printf("data insertion for debugging completed!\n");            
        }
    }
    
    printf("main thread is terminated!\n");

    return 0;
}
