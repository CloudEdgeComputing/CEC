#include "FACTORYBUILDER.h"
#include "BASICCELL.h"
#include "PIPE.h"
#include "STREAMFACTORY.h"
#include "Functions.h"
#include "TUPLE.h"
#include "MIGRATION.h"
#include "SRCCELL.h"
#include "DESTCELL.h"
#include "UNIONCELL.h"

// Task, Connection에 outq는 2개 이상이 붙을 수 있고, inq는 항상 하나다.
STREAMFACTORY* FACTORYBUILDER::createSTREAMFACTORY ( ushort recvport, MIGRATION* mig)
{

    // 익스큐터 인스턴스 생성 및 등록
    printf ( "streamfactory instance creation...\n" );
    STREAMFACTORY* streamfactory = new STREAMFACTORY(mig);
    this->streamfactories.push_back ( streamfactory );

    printf ( "SRCCELL creation...\n" );
    PIPE* srcpipe1 = this->makePIPE ( UNDEFINED, TYPE_CELL, streamfactory );
    SRCCELL* srccell1 = new SRCCELL ( recvport, srcpipe1, streamfactory );
    
    PIPE* srcpipe2 = this->makePIPE ( UNDEFINED, TYPE_CELL, streamfactory );
    SRCCELL* srccell2 = new SRCCELL ( recvport + 1, srcpipe2, streamfactory );

    printf ( "DESTCELL creation...\n" );
    PIPE* destpipe = this->makePIPE ( UNDEFINED, TYPE_CELL, streamfactory );
    DESTCELL* destcell = new DESTCELL ( destpipe, streamfactory );

    printf ( "BASICCELL creation...\n" );
    // 차량 쿼리
    FUNC func1 = cartask;

    BASICCELL* cell1 = new BASICCELL ( srcpipe1, destpipe, 2, func1, streamfactory );
    
    // 인간 등록
    FUNC func2 = pedtask;
    PIPE* outpipe = NULL;
    BASICCELL* cell2 = new BASICCELL ( srcpipe2, outpipe, 2, func2, streamfactory );

    printf ( "Operation creation for each task...\n" );
    // Task 내부 오퍼레이터 생성
    srccell1->makeWorker();
    srccell2->makeWorker();
    cell1->makeWorker();
    cell2->makeWorker();
    destcell->makeWorker();


    printf ( "Cell registering to factory instance...\n" );
    // 생성된 task를 executor에 등록시킴
    streamfactory->registerCELL ( srccell1 );
    streamfactory->registerCELL ( srccell2 );
    streamfactory->registerCELL ( cell1 );
    streamfactory->registerCELL ( cell2 );
    streamfactory->registerCELL ( destcell );

    // 스크립트 해석 끝
}

void FACTORYBUILDER::runSTREAMFACTORY ( STREAMFACTORY* factory )
{
    printf ( "registered factory: %d\n", this->streamfactories.size() );
    for ( auto iter = this->streamfactories.begin(); iter != this->streamfactories.end(); ++iter )
    {
        STREAMFACTORY* factory = *iter;
        factory->factoryStart();
    }
}

STREAMFACTORY* FACTORYBUILDER::getStreamFactorybyid ( int id )
{
    int cnt = 0;

    for ( auto iter = this->streamfactories.begin(); iter != this->streamfactories.end(); ++iter )
    {
        if ( cnt == id )
            return *iter;
        else
            cnt++;
    }
    printf ( "Unrecognized executor id!\n" );
    exit ( 0 );
    return NULL;
}

PIPE* FACTORYBUILDER::makePIPE ( void* powner, int type, STREAMFACTORY* parent )
{
    int id = this->new_gid++;
    PIPE* pipe = new PIPE ( new quetype, powner, type, id, parent );
    this->pipes.push_back ( pipe );

    return pipe;
}

PIPE* FACTORYBUILDER::findPIPE ( unsigned int id )
{
    for ( auto iter = pipes.begin(); iter != pipes.end(); ++iter )
    {
        PIPE* pipe = *iter;
        if ( id == pipe->getid() )
            return pipe;
    }

    printf ( "Queue is not found!\n" );
    exit ( 0 );
    return NULL;
}

