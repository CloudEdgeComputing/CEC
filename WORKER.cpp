#include "WORKER.h"
#include "PIPE.h"
#include "BASICCELL.h"
#include "TUPLE.h"
#include "UNIONCELL.h"
#include <STREAMFACTORY.h>

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

    switch ( worker->getParentCELL()->getXIXO() )
    {
        case SISO:
        {
            list<PIPE*>* outpipelist;

            BASICCELL* cell = ( BASICCELL* ) worker->getParentCELL();

            outpipelist = cell->getoutpipelist();

            while ( 1 )
            {
                // 인풋큐가 비었거나 셀 스테이트가 false인경우
                bool besleep = cell->getinpipe()->empty() || !cell->getCELLState();

                if ( besleep )
                {
                    //printf("SISO sleeping...\n");
                    worker->sleep();
                    //printf("SISO wakeup... \n");
                    continue;
                }

                TUPLE* tuple;

                if ( ( tuple = cell->getinpipe()->pop() ) == NULL )
                {
                    continue;
                }
                
                worker->setuuid(tuple->getuuid());

                // Sequence check TODO
                //debug_packet(data->getcontent(), (unsigned int)data->getLen());

                // function call using data

                FUNC func = ( FUNC ) cell->getfunc();
                //printf("processing!\n");
                TUPLE* output = func ( tuple, worker->getParentCELL()->getParentFactory()->getStatemanager() );
                
                
                if(output == NULL)
                    continue;

                // uuid는 변하지 않는다.
                output->setuuid ( tuple->getuuid() );

                // outq로 결과 데이터를 보낸다.
                for ( auto iter = outpipelist->begin(); iter != outpipelist->end(); ++iter )
                {
                    auto outpipe = *iter;

                    outpipe->push ( output );
                }
                worker->clearuuid();

            }

            break;
        }
        case MISO:
        {
            list<PIPE*>* inpipelist;
            list<PIPE*>* outpipelist;

            UNIONCELL* cell = ( UNIONCELL* ) worker->getParentCELL();

            inpipelist = cell->getinpipelist();
            outpipelist = cell->getoutpipelist();

            while ( 1 )
            {
                // 여기는 inpipe가 모두 데이터를 가지고 있음을 보장 받아야 하고
                // 보장 받은 순간 이 워커만 데이터들을 뽑아낼 수 있다.
                pthread_mutex_lock ( cell->getpipelock() );

                // 튜플을 보관하기 위한 임시장소
                list<TUPLE*>* intuples = new list<TUPLE*>;

                // pipe_ready: 하나 이상의 파이프는 데이터를 담고 있어야 함
                // newest_ready: 데이터를 담고 있지 않은 파이프는 newest가 있어야 함
                // besleep: 위 두 ready 중 하나 이상 만족 하지 않아야 함 + cell state가 러닝이 안되어 있어야 함

                bool pipe_ready = false;
                bool newest_ready = true;

                if ( cell->getUnionPolicy() == POLICY_PARTIALREADY )
                {
                    for ( auto iter = inpipelist->begin(); iter != inpipelist->end(); ++iter )
                    {
                        PIPE* pipe = *iter;
                        // 파이프 큐에 데이터가 있거나 전에 보냈던 데이터가 있는 경우
                        if ( pipe->getQueue()->empty() == false )
                        {
                            pipe_ready |= true;
                        }
                        else
                        {
                            if ( pipe->getnewest ( false ) != false )
                            {
                                newest_ready &= true;
                            }
                            else
                            {
                                newest_ready = false;
                            }
                        }
                    }
                }
                else if ( cell->getUnionPolicy() == POLICY_ALLREADY )
                {
                    pipe_ready = true;
                    for ( auto iter = inpipelist->begin(); iter != inpipelist->end(); ++iter )
                    {
                        PIPE* pipe = *iter;
                        // 파이프 큐에 데이터가 있거나 전에 보냈던 데이터가 있는 경우
                        newest_ready &= !pipe->getQueue()->empty();
                    }
                }
                
                bool besleep = !cell->getCELLState() || ! ( pipe_ready && newest_ready );

                if ( besleep )
                {
                    //printf("MISO sleeping...\n");
                    pthread_mutex_unlock ( cell->getpipelock() );
                    worker->sleep();
                    //printf("MISO wakeup... \n");
                    continue;
                }

                for ( auto iter = inpipelist->begin(); iter != inpipelist->end(); ++iter )
                {
                    PIPE* pipe = *iter;

                    TUPLE* tuple;
                    if ( ( tuple = pipe->pop() ) == NULL )
                    {
                        // 데이터가 없는 경우 newest data를 긁어 온다.
                        TUPLE* pasttuple = pipe->getnewest ( true );
                        pasttuple->setpipeid ( pipe->getid() );
                        intuples->push_back ( pasttuple );
                    }
                    else
                    {
                        // 데이터가 있는 경우 현재 tuple에 데이터가 있음
                        tuple->setpipeid ( pipe->getid() );
                        intuples->push_back ( tuple );
                        // newest tuple로 복사함
                        pipe->setnewest ( tuple );

                    }
                }

                pthread_mutex_unlock ( cell->getpipelock() );
                for(auto iter = intuples->begin(); iter != intuples->end(); ++iter)
                {
                    TUPLE* tuple = *iter;
                    worker->setuuid(tuple->getuuid());
                }

                MERGE_FUNC func = cell->getfunc();

                TUPLE* output = func ( intuples, cell->getdominantpipeid() );

                delete intuples;

                for ( auto iter = outpipelist->begin(); iter != outpipelist->end(); ++iter )
                {
                    auto outpipe = *iter;

                    outpipe->push ( output );
                }
                worker->clearuuid();
            }
            break;
        }
        default:
        {
            printf ( "Unexpected XIXO type!\n" );
            exit ( 0 );
            break;
        }
    }
}

CELL* WORKER::getParentCELL()
{
    return this->parentCELL;
}

void WORKER::setuuid ( unsigned int uuid )
{
    this->uuid.push_back(uuid);
}

bool WORKER::hasuuid( unsigned int uuid )
{
    for(auto iter = this->uuid.begin(); iter != this->uuid.end(); ++iter)
    {
        unsigned int _uuid = *iter;
        if(_uuid == uuid)
            return true;
    }
    return false;
}

void WORKER::clearuuid()
{
    this->uuid.clear();
}