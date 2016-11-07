#include "DESTCELL.h"
#include "PIPE.h"
#include "TUPLE.h"
#include "STREAMFACTORY.h"

DESTCELL::DESTCELL ( list< PIPE* >* inpipelist, STREAMFACTORY* parent ) : CELL ( inpipelist, 1, parent, SI )
{
}

DESTCELL::DESTCELL ( PIPE* inpipe, STREAMFACTORY* parent ) : CELL ( inpipe, 1, parent, SI )
{
}

void DESTCELL::makeWorker()
{
    // 실제로는 센더를 만들어야 함... 근데 사실 할일 없음.
}

// 센더를 실행시키는 함수
void* DESTCELL::scheduling ( void* arg )
{
    pthread_t sender_tid;
    struct CONNDATA* senderdata = new struct CONNDATA;
    senderdata->fd = 0;
    senderdata->inpipelist = this->inpipelist;
    senderdata->thispointer = this;
    pthread_create ( &sender_tid, NULL, &DESTCELL::DESTbroker_start_wrapper, ( void* ) senderdata );
}

void* DESTCELL::DESTbroker_start_wrapper ( void* context )
{
    struct CONNDATA* conndata = ( struct CONNDATA* ) context;
    void* thispointer = conndata->thispointer;
    return ( ( DESTCELL* ) thispointer )->DESTbroker_start_internal ( context );
}

void* DESTCELL::DESTbroker_start_internal ( void* context )
{
    struct CONNDATA* conndata = ( struct CONNDATA* ) context;
    
    while(1)
    {
        // 태스크 스케줄링 블럭 요청이 있으면 블럭한다.
        if ( !this->isrun )
        {
            printf ( "DESTCELL blocked!\n" );
            pthread_cond_wait ( &this->condition, &this->mutex );
            printf ( "DESTCELL released!\n" );
        }
        
        // Round robin
        for(auto iter = this->inpipelist->begin(); iter != this->inpipelist->end(); ++iter)
        {
            PIPE* pipe = *iter;
            lockfreeq* inq = pipe->getQueue();
            
            if(inq->empty())
                continue;
            
            TUPLE* tuple;
            
            if(!inq->pop(tuple))
            {
                printf("inq pop error in %s\n", __func__);
                exit(0);
            }
            
            if(tuple->getcontent() == NULL)
            {
                printf("No content in %s\n", __func__);
            }
            
            if(send(tuple->getfd(), tuple->getdata(), tuple->getLen() + 4, 0) == -1)
            {
                printf("Error occured in send function in %s\n", __func__);
                while(1);
            }
            
            delete[] tuple->getdata();
            delete tuple;
        }
    }
    return NULL;
}