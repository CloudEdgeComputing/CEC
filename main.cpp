#include <stdio.h>
#include "FACTORYBUILDER.h"
#include "MIGRATION.h"
#include "STREAMFACTORY.h"
#include "BASICCELL.h"
#include "TUPLE.h"
#include "PIPE.h"
#include <signal.h>

using namespace std;

FACTORYBUILDER factorymanager;


int main ( int argc, char **argv )
{
    signal(SIGPIPE, SIG_IGN);
    
    printf ( "Put 's' for server, 'c' for client\n" );
    char cp;
    scanf ( "%c", &cp );

    if ( cp == 's' )
    {
        MIGRATION Mig ( 1200, cp );
        factorymanager.setMIGRATION ( &Mig );
        // 익스큐터 생성 후, 내부에서 인자를 이용하여 커넥션을 생성

        STREAMFACTORY* factory = factorymanager.createSTREAMFACTORY ( 1237 );

        // 등록된 모든 익스큐터 런런런
        factorymanager.runSTREAMFACTORY(factory);
    }
    else
    {
        MIGRATION Mig ( 1200, cp );
        factorymanager.setMIGRATION ( &Mig );
        // 익스큐터 생성 후, 내부에서 인자를 이용하여 커넥션을 생성

        STREAMFACTORY* factory = factorymanager.createSTREAMFACTORY ( 1239 );

        // 등록된 모든 익스큐터 런런런
        factorymanager.runSTREAMFACTORY(factory);
    }

    printf ( "completion of basis\n" );


    // For debug
    while ( 1 )
    {
        scanf ( "%c", &cp );
        if ( cp == 'd' )
            factorymanager.startMIGRATION();
    }

    return 0;
}
