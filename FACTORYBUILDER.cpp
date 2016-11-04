#include "FACTORYBUILDER.h"
#include "BASICCELL.h"
#include "PIPE.h"
#include "STREAMFACTORY.h"
#include "CONNECTION.h"
#include "Functions.h"
#include "TUPLE.h"
#include "MIGRATION.h"

// Task, Connection에 outq는 2개 이상이 붙을 수 있고, inq는 항상 하나다.
STREAMFACTORY* FACTORYBUILDER::createSTREAMFACTORY ( ushort recvport )
{
    // 익스큐터 인스턴스 생성 및 등록
    printf ( "streamfactory instance creation...\n" );
    STREAMFACTORY* streamfactory = new STREAMFACTORY;
    this->streamfactories.push_back ( streamfactory );

    // 익스큐터 전체의 인풋 아웃풋 큐 생성
    printf ( "STREAMFACTORY inputqueue and outputqueue creation...\n" );
    streamfactory->setinpipe ( this->makePIPE ( UNDEFINED, TYPE_CONNECTION ) );
    printf ( "global id for inq: %d\n", streamfactory->getinpipe()->getid() );
    streamfactory->setoutpipe ( this->makePIPE ( streamfactory, TYPE_FACTORY ) );

    auto outpipelist = streamfactory->getoutpipelist();
    for ( auto iter = outpipelist->begin(); iter != outpipelist->end(); ++iter )
    {
        PIPE* pipe = *iter;
        printf ( "global id for outq: %d\n", pipe->getid() );
    }

    // 익스큐터에 연결할 커넥션 생성
    printf ( "STREAMFACTORY connection setting...\n" );
    streamfactory->setCONNECTOR ( new CONNECTION ( recvport, streamfactory ) );
    streamfactory->getCONNECTOR()->serverStart ( streamfactory->getinpipe(), streamfactory->getoutpipelist() );

    printf ( "BASICCELL creation...\n" );
    /*
     * 들어오는 함수는 인자로 DATA* 를 써야 하며, DATA*를 쓴 후에는 꼭 메모리 해제가 필요하다.
     */
    // Script 해석부
    FUNC func1 = task_1;
    FUNC func2 = task_2;
    printf ( "registred func1 %p, %p\n", func1, func2 );

    // Task 생성
    PIPE* task1pipe = this->makePIPE ( UNDEFINED, TYPE_CELL );
    printf ( "global id for task1pipe : %d\n", task1pipe->getid() );
    BASICCELL* cell1 = new BASICCELL ( streamfactory->getinpipe(), task1pipe, 2, func1, streamfactory );
    BASICCELL* cell2 = new BASICCELL ( task1pipe, streamfactory->getoutpipelist(), 2, func2, streamfactory );
    printf ( "registered task1 %p, %p\n", cell1, cell2 );

    printf ( "Operation creation for each task...\n" );
    // Task 내부 오퍼레이터 생성
    cell1->makeWorker();
    cell2->makeWorker();

    printf ( "Task registering to executor instance...\n" );
    // 생성된 task를 executor에 등록시킴
    streamfactory->registerCELL ( cell1 );
    streamfactory->registerCELL ( cell2 );

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

PIPE* FACTORYBUILDER::makePIPE ( void* powner, int type )
{
    int id = this->new_gid++;
    PIPE* pipe = new PIPE ( new lockfreeq ( 0 ), powner, type, id );
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

void FACTORYBUILDER::setMIGRATION(MIGRATION* mig)
{
    this->migration = mig;
}

MIGRATION* FACTORYBUILDER::getMIGRATION()
{
    return this->migration;
}

void FACTORYBUILDER::startMIGRATION()
{
    this->migration->startMIGRATION ( 0, *this->streamfactories.begin() );
}

