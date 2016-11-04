#include "WORKER.h"
#include "PIPE.h"
#include "BASICCELL.h"
#include "TUPLE.h"
#include "UNIONCELL.h"

WORKER::WORKER ( CELL* parent )
{
    this->parentCELL = parent;
    this->working = false;
    this->mutex = PTHREAD_MUTEX_INITIALIZER;
    this->condition = PTHREAD_COND_INITIALIZER;
}

bool WORKER::isWorking()
{
    return this->working;
}

void WORKER::create()
{
    // 런 상태가 안되어있으면 wrapper는 self blocking함
    // 런 상태가 되어있으면 만들자마자 돌아감
    pthread_create ( &this->tid, NULL, wrapper_thread, ( void* ) this );
}

void WORKER::wakeup()
{
    this->working = true;
    pthread_cond_signal ( &this->condition );
}

void WORKER::sleep()
{
    this->working = false;;
    pthread_cond_wait ( &this->condition, &this->mutex );
    pthread_mutex_unlock ( &this->mutex );
}

// wrapper_thread를 이용하여 오퍼레이터 함수 실행시키기
static void* wrapper_thread ( void* arg )
{
    WORKER* worker = ( WORKER* ) arg;
    
    switch(worker->getParentCELL()->getXIXO())
    {
        case SISO:
        {
            lockfreeq* inpipeq;
            list<PIPE*>* outpipelist;
            
            BASICCELL* cell = (BASICCELL*)worker->getParentCELL();
            
            inpipeq = cell->getinpipe()->getQueue();
            outpipelist = cell->getoutpipelist();
            
            while( 1 )
            {
                // 인풋큐가 비었거나 셀 스테이트가 false인경우
                bool besleep = inpipeq->empty() || !cell->getCELLState();
                
                if( besleep )
                {
                    //printf("SISO sleeping...\n");
                    worker->sleep();
                    //printf("SISO wakeup... \n");
                    continue;
                }
                
                TUPLE* tuple;
                
                if ( !inpipeq->pop ( tuple ) )
                {
                    continue;
                }

                // Sequence check TODO
                //debug_packet(data->getcontent(), (unsigned int)data->getLen());

                // function call using data

                FUNC func = ( FUNC ) cell->getfunc();
                //printf("processing!\n");
                TUPLE* output = func ( tuple );

                // 오너 소켓디스크립터는 변하지 않았으므로 유지한다.
                output->setfd ( tuple->getfd() );

                // outq로 결과 데이터를 보낸다.
                for ( auto iter = outpipelist->begin(); iter != outpipelist->end(); ++iter )
                {
                    auto outpipeq = ( *iter )->getQueue();

                    if ( !outpipeq->push ( output ) )
                    {
                        printf ( "Error occured in wrapper thread push!\n" );
                        exit ( 0 );
                    }
                }
                
            }
            
            break;
        }
        case MISO:
        {
            list<PIPE*>* inpipelist;
            list<PIPE*>* outpipelist;
            
            UNIONCELL* cell = (UNIONCELL*)worker->getParentCELL();
            
            inpipelist = cell->getinpipelist();
            outpipelist = cell->getoutpipelist();
            
            while(1)
            {
                bool allready = false;
                
                // 여기는 inpipe가 모두 데이터를 가지고 있음을 보장 받아야 하고 
                // 보장 받은 순간 이 워커만 데이터들을 뽑아낼 수 있다. 
                pthread_mutex_lock(cell->getpipelock());
                
                list<TUPLE*>* intuples = new list<TUPLE*>;
                
                for(auto iter = inpipelist->begin(); iter != inpipelist->end(); ++iter)
                {
                    PIPE* pipe = *iter;
                    allready &= !pipe->getQueue()->empty();
                }
                
                bool besleep = !allready || !cell->getCELLState();
                
                if( besleep )
                {
                    //printf("SISO sleeping...\n");
                    pthread_mutex_unlock(cell->getpipelock());
                    worker->sleep();
                    //printf("SISO wakeup... \n");
                    continue;
                }
                
                for(auto iter = inpipelist->begin(); iter != inpipelist->end(); ++iter)
                {
                    PIPE* pipe = *iter;
                    
                    TUPLE* tuple;
                    if(!pipe->getQueue()->pop ( tuple ))
                    {
                        printf("MISO pipelock does not working appropriately\n");
                        exit(0);
                    }
                    
                    intuples->push_back(tuple);
                }
                
                pthread_mutex_unlock(cell->getpipelock());
                
                MERGE_FUNC func = cell->getfunc();
                
                TUPLE* output = func ( intuples, cell->getdominantpipeid() );
                
                delete intuples;
                
                for ( auto iter = outpipelist->begin(); iter != outpipelist->end(); ++iter )
                {
                    auto outpipeq = ( *iter )->getQueue();

                    if ( !outpipeq->push ( output ) )
                    {
                        printf ( "Error occured in wrapper thread push!\n" );
                        exit ( 0 );
                    }
                }
            }
            break;
        }
        default:
        {
            printf("Unexpected XIXO type!\n");
            exit(0);
            break;
        }
    }
}

CELL* WORKER::getParentCELL()
{
    return this->parentCELL;
}
