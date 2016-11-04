#include "UNIONCELL.h"
#include "PIPE.h"
#include "STREAMFACTORY.h"
#include "WORKER.h"
#include "TUPLE.h"

UNIONCELL::UNIONCELL ( list< PIPE* >* inpipelist, uint dominantpipeid, std::__cxx11::list< PIPE* >* outpipelist, ushort count, STREAMFACTORY* parent ) : CELL ( inpipelist, outpipelist, count, parent, MISO )
{    
    // Internal merge 함수를 쓴다.
    this->func = merge;
    this->dominantpipeid = dominantpipeid;
    this->pipelock = PTHREAD_MUTEX_INITIALIZER;
    
}

void UNIONCELL::makeWorker()
{
    // Operator를 count만큼 만든다.
    for ( int i = 0; i < this->count; i++ )
    {
        WORKER* worker = new WORKER ( this );
        this->workers.push_back ( worker );
    }

    // CELL를 false시키고 operator를 런 시키면 모든 task는 생성 뒤, 블럭 상태가 된다.
    for ( auto iter = workers.begin(); iter != workers.end(); ++iter )
    {
        WORKER* worker = *iter;
        worker->create();
    }
}

void* UNIONCELL::scheduling ( void* arg )
{
    while ( 1 )
    {
        // 태스크 스케줄링 종료 요청이 있으면 종료한다.
        if ( this->isEnd )
        {
            printf ( "exit in UNIONCELL scheduling!\n" );
            exit ( 0 );
        }

        // 태스크 스케줄링 블럭 요청이 있으면 블럭한다.
        if ( !this->isrun )
        {
            printf ( "UNIONCELL scheduler blocked!\n" );
            pthread_cond_wait ( &this->condition, &this->mutex );
            printf ( "UNIONCELL scheduler released!\n" );
        }
        
        // UNIONCELL은 MISO를 표방하므로 inpipelist를 전부 찾아내고 그 데이터들을 한번에 꺼낼 수 있도록 한다.
        bool allready = false;
        for(auto iter = this->inpipelist->begin(); iter != this->inpipelist->end(); ++iter)
        {
            PIPE* pipe;
            allready &= !pipe->getQueue()->empty();
        }
        
        if(allready == true)
        {
            for (auto iter = this->workers.begin() ; iter != this->workers.end(); ++iter )
            {
                WORKER* worker = *iter;
                if ( !worker->isWorking() )
                {
                    worker->wakeup();
                }
            }
        }
    }
}

list< PIPE* >* UNIONCELL::getinpipelist()
{
    return this->inpipelist;
}

MERGE_FUNC UNIONCELL::getfunc()
{
    return this->func;
}

pthread_mutex_t* UNIONCELL::getpipelock()
{
    return &this->pipelock;
}

uint UNIONCELL::getdominantpipeid()
{
    return this->dominantpipeid;
}

TUPLE* merge(list< TUPLE* >* tuplelist, uint dominantpipeid )
{
    // 튜플들의 데이터 사이즈 합
    uint size = 0;
    TUPLE* dominantTuple = NULL;
    for(auto iter = tuplelist->begin(); iter != tuplelist->end(); ++iter)
    {
        TUPLE* tuple = *iter;
        size += tuple->getLen();
        
        if(tuple->getpipeid() == dominantpipeid)
            dominantTuple = tuple;
    }
    
    if(dominantTuple == NULL)
    {
        return new TUPLE(0, 0, 0);
    }
    
    TUPLE* new_tuple = new TUPLE(size, dominantTuple->getfd(), dominantTuple->gettype());
    
    for(auto iter = tuplelist->begin(); iter != tuplelist->end(); ++iter)
    {
        TUPLE* tuple = *iter;
        new_tuple->push(tuple->getdata(), tuple->getLen());
        //튜플 내부 데이터 삭제 금지!!!!! (Important)
        delete tuple;
    }
    
    new_tuple->sealing();
    
    return new_tuple;
}
