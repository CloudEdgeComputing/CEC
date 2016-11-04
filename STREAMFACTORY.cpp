#include "STREAMFACTORY.h"
#include "CONNECTION.h"
#include "BASICCELL.h"
#include "PIPE.h"
#include "TUPLE.h"
#include "FACTORYBUILDER.h"
#include "Debug.h"

STREAMFACTORY::STREAMFACTORY()
{
    this->outpipelist = new list<PIPE*>;
}

void STREAMFACTORY::factoryStart()
{
    printf ( "For an factory, registered cell: %d\n", this->cells.size() );
    for ( auto iter = this->cells.begin(); iter != this->cells.end(); ++iter )
    {
        BASICCELL* cell = *iter;
        printf ( "cell %p is started!\n", cell );
        cell->schedulingStart();
    }
}

CONNECTION* STREAMFACTORY::getCONNECTOR()
{
    return this->conn;
}

void STREAMFACTORY::setCONNECTOR ( CONNECTION* conn )
{
    this->conn = conn;
}

void STREAMFACTORY::setinpipe ( PIPE* inpipe )
{
    this->inpipe = inpipe;
    inpipe->registerforwardDependency ( this, TYPE_FACTORY );
}

void STREAMFACTORY::setoutpipe ( PIPE* outpipe )
{
    this->outpipelist->push_back ( outpipe );
    outpipe->registerbackDependency ( this, TYPE_FACTORY );
}

PIPE* STREAMFACTORY::getinpipe()
{
    return this->inpipe;
}

list<PIPE*>* STREAMFACTORY::getoutpipelist()
{
    return this->outpipelist;
}

list< BASICCELL* > STREAMFACTORY::getCELLs()
{
    return this->cells;
}

void STREAMFACTORY::registerCELL ( BASICCELL* cell )
{
    this->cells.push_back ( cell );
}

bool STREAMFACTORY::getSTREAMFACTORYState()
{
    return this->state;
}

bool STREAMFACTORY::setSTREAMFACTORYState ( bool state )
{
    this->state = state;
}

void STREAMFACTORY::installReceivedData ( TUPLE* data )
{
    if ( !data->validity() )
    {
        printf ( "Migrated tuple is not valid!\n" );
        return;
    }

    // 데이터 분해
    char* p = data->getcontent();
    ushort size = 0;

    debug_packet ( data->getcontent(), data->getLen() + 4 );

    // 오직 한번에 하나의 큐 데이터만 들어옴
    PIPE* pipe = NULL;

    if ( data->getLen() != 0 )
    {

        // 단위 데이터를 읽어 설치함
        while ( size < data->getLen() )
        {
            unsigned int pipeid = 0;
            memcpy ( &pipeid, p + 8, 4 );

            // unit data가 스페셜 형식을 가짐
            TUPLE* unitdata = new TUPLE ( p, true );

            printf ( "queue id: %d\n", pipeid );
            //printf("received packet!\n");
            //debug_packet(unitdata->getdata(), unitdata->getLen() + 4);

            // qid를 이용해 queue를 얻어옴
            pipe = factorymanager.findPIPE ( pipeid );

            // queue에 데이터를 푸시함
            pipe->getQueue()->push ( unitdata );

            p = p + unitdata->getLen() + 8 + 4;
            size += unitdata->getLen() + 8 + 4;
            //printf("size: %d Len: %d\n", size, data->getLen());
            //printf("unitdata->getLen() + 8: %d\n", unitdata->getLen() + 8 + 4);
        }

        pipe->install_comp_signal();
    }
}

void STREAMFACTORY::printtasks()
{
    for ( auto iter = this->cells.begin(); iter != this->cells.end(); ++iter )
    {
        BASICCELL* cell = *iter;
        printf ( "cell: %p\n", cell );
    }
    printf ( "end!\n" );
}
