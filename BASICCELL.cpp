#include "BASICCELL.h"
#include "WORKER.h"
#include "PIPE.h"
#include "STREAMFACTORY.h"

BASICCELL::BASICCELL ( PIPE* inpipe, PIPE* outpipe, ushort count, FUNC func, STREAMFACTORY* parent ) : CELL ( inpipe, outpipe, count, parent, SISO)
{
    this->func = func;
}

BASICCELL::BASICCELL ( PIPE* inpipe, list< PIPE* >* outpipelist, ushort count, FUNC func, STREAMFACTORY* parent ) : CELL ( inpipe, outpipelist, count, parent, SISO )
{
    this->func = func;
}

void BASICCELL::makeWorker()
{
    // Operator를 count만큼 만든다.
    for ( int i = 0; i < this->count; i++ )
    {
        WORKER* worker = new WORKER ( this );
        this->workers.push_back ( worker );
    }

    // BASICCELL를 false시키고 operator를 런 시키면 모든 task는 생성 뒤, 블럭 상태가 된다.
    for ( auto iter = workers.begin(); iter != workers.end(); ++iter )
    {
        WORKER* worker = *iter;
        worker->create();
    }
}

void* BASICCELL::scheduling ( void* arg )
{
    while ( 1 )
    {
        // 태스크 스케줄링 종료 요청이 있으면 종료한다.
        if ( this->isEnd )
        {
            printf ( "exit in cell scheduling!\n" );
            exit ( 0 );
        }

        // 태스크 스케줄링 블럭 요청이 있으면 블럭한다.
        if ( !this->isrun )
        {
            printf ( "BASICCELL scheduler blocked!\n" );
            pthread_cond_wait ( &this->condition, &this->cond_mutex );
            printf ( "BASICCELL scheduler released!\n" );
        }


        // BASICCELL은 SISO를 표방하므로 inpipelist의 첫번째만 알면 된다.
        PIPE* inpipe = * ( this->inpipelist->begin() );
        if ( !inpipe->empty() )
        {
            for (auto iter = this->workers.begin() ; iter != this->workers.end(); ++iter )
            {
                WORKER* worker = *iter;
                //printf("op->getinUse: %d\n", op->getinUse());
                if ( !worker->isWorking() )
                {
                    //printf("woke up!\n");
                    worker->wakeup();
                }
            }
        }
    }
}

PIPE* BASICCELL::getinpipe()
{
    PIPE* inpipe = * ( this->inpipelist->begin() );
    return inpipe;
}

FUNC BASICCELL::getfunc()
{
    return this->func;
}