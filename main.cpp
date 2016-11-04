#include <stdio.h>
#include "FACTORYBUILDER.h"
#include "MIGRATION.h"
#include "STREAMFACTORY.h"
#include "BASICCELL.h"
#include "TUPLE.h"
#include "PIPE.h"
#include "CONNECTION.h"

using namespace std;

FACTORYBUILDER factorymanager;


int main ( int argc, char **argv )
{
    
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
        else if ( cp == 'p' )
        {
            STREAMFACTORY* factory = factorymanager.getStreamFactorybyid(0);
                            // 모든 Task를 블럭한다.
            auto list = factory->getCELLs();
            for ( auto iter = list.begin(); iter != list.end(); ++iter )
            {
                BASICCELL* cell = *iter;
                cell->schedulerSleep();
            }

            // Conection을 블럭한다.
            CONNECTION* conn = factory->getCONNECTOR();
            conn->sleepCONNECTOR();

                            // 데이터를 삽입한다.
            int value = 4;
            TUPLE* data = new TUPLE ( 4, 1, 0 );
            data->push ( &value, 4 );
            data->sealing();

            int value2 = 5;
            TUPLE* data2 = new TUPLE ( 4, 1, 0 );
            data2->push ( &value2, 4 );
            data2->sealing();

            for ( auto iter = list.begin(); iter != list.end(); ++iter )
            {
                BASICCELL* cell = *iter;
                lockfreeq* que = cell->getinpipe()->getQueue();
                que->push ( data );
                que->push ( data2 );
            }
            printf ( "data insertion for debugging completed!\n" );
        }
    }

    return 0;
}
