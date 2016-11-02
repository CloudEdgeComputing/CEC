#include <stdio.h>
#include "ExecutorManager.h"
#include "Migration.h"

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
    
    while(1);
    
    printf("main thread is terminated!\n");

    return 0;
}
