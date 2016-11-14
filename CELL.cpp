#include "CELL.h"
#include "PIPE.h"
#include "WORKER.h"
#include "STREAMFACTORY.h"
#include <string.h>

CELL::CELL ( PIPE* inpipe, list< PIPE* >* outpipes, ushort count, STREAMFACTORY* parent, uint XIXO )
{
    this->inpipelist = new list<PIPE*>;
    this->outpipelist = new list<PIPE*>;

    if ( inpipe != NULL )
    {
        this->inpipelist->push_back ( inpipe );
        inpipe->registerforwardDependency ( this, TYPE_CELL );
    }

    for ( auto iter = outpipes->begin(); iter != outpipes->end(); ++iter )
    {
        PIPE* pipe = *iter;
        pipe->registerbackDependency ( this, TYPE_CELL );
        this->outpipelist->push_back ( pipe );
    }

    this->count = count;
    this->parent = parent;
    this->isrun = true;
    this->isEnd = false;
    this->XIXO = XIXO;
    this->mutex = PTHREAD_MUTEX_INITIALIZER;
    this->condition = PTHREAD_COND_INITIALIZER;
}

CELL::CELL ( PIPE* inoutpipe, ushort count, STREAMFACTORY* parent, uint XIXO )
{
    if ( XIXO == SO )
    {
        this->inpipelist = NULL;
        this->outpipelist = new list<PIPE*>;

        inoutpipe->registerbackDependency ( this, TYPE_CELL );
        this->outpipelist->push_back ( inoutpipe );

        this->count = count;
        this->parent = parent;
        this->isrun = true;
        this->isEnd = false;
        this->XIXO = XIXO;
        this->mutex = PTHREAD_MUTEX_INITIALIZER;
        this->condition = PTHREAD_COND_INITIALIZER;
    }
    else if ( XIXO == SI )
    {
        this->inpipelist = new list<PIPE*>;
        this->outpipelist = NULL;

        inoutpipe->registerforwardDependency ( this, TYPE_CELL );
        this->inpipelist->push_back ( inoutpipe );

        this->count = count;
        this->parent = parent;
        this->isrun = true;
        this->isEnd = false;
        this->XIXO = XIXO;
        this->mutex = PTHREAD_MUTEX_INITIALIZER;
        this->condition = PTHREAD_COND_INITIALIZER;
    }
}

CELL::CELL ( PIPE* inpipe, PIPE* outpipe, ushort count, STREAMFACTORY* parent, uint XIXO )
{
    this->inpipelist = new list<PIPE*>;
    this->outpipelist = new list<PIPE*>;

    if ( outpipe != NULL )
    {
        this->outpipelist->push_back ( outpipe );
        outpipe->registerbackDependency ( this, TYPE_CELL );
    }

    if ( inpipe != NULL )
    {
        this->inpipelist->push_back ( inpipe );
        inpipe->registerforwardDependency ( this, TYPE_CELL );
    }

    this->count = count;
    this->parent = parent;
    this->isrun = true;
    this->isEnd = false;
    this->XIXO = XIXO;
    this->mutex = PTHREAD_MUTEX_INITIALIZER;
    this->condition = PTHREAD_COND_INITIALIZER;
}

CELL::CELL ( list< PIPE* >* inoutpipes, ushort count, STREAMFACTORY* parent, uint XIXO )
{
    if ( XIXO == SO )
    {
        this->inpipelist = NULL;
        this->outpipelist = new list<PIPE*>;

        for ( auto iter = inoutpipes->begin(); iter != inoutpipes->end(); ++iter )
        {
            PIPE* pipe = *iter;
            pipe->registerbackDependency ( this, TYPE_CELL );
            this->outpipelist->push_back ( pipe );
        }

        this->count = count;
        this->parent = parent;
        this->isrun = true;
        this->isEnd = false;
        this->XIXO = XIXO;
        this->mutex = PTHREAD_MUTEX_INITIALIZER;
        this->condition = PTHREAD_COND_INITIALIZER;
    }
    else if ( XIXO == SI )
    {
        this->inpipelist = new list<PIPE*>;
        this->outpipelist = NULL;

        for ( auto iter = inoutpipes->begin(); iter != inoutpipes->end(); ++iter )
        {
            PIPE* pipe = *iter;
            pipe->registerbackDependency ( this, TYPE_CELL );
            this->inpipelist->push_back ( pipe );
        }

        this->count = count;
        this->parent = parent;
        this->isrun = true;
        this->isEnd = false;
        this->XIXO = XIXO;
        this->mutex = PTHREAD_MUTEX_INITIALIZER;
        this->condition = PTHREAD_COND_INITIALIZER;
    }
}

CELL::CELL ( list< PIPE* >* inpipelist, list< PIPE* >* outpipelist, ushort count, STREAMFACTORY* parent, uint XIXO )
{
    this->inpipelist = new list<PIPE*>;
    this->outpipelist = new list<PIPE*>;

    for ( auto iter = inpipelist->begin(); iter != inpipelist->end(); ++iter )
    {
        PIPE* pipe = *iter;
        pipe->registerforwardDependency ( this, TYPE_CELL );
        this->inpipelist->push_back ( pipe );
    }

    for ( auto iter = outpipelist->begin(); iter != outpipelist->end(); ++iter )
    {
        PIPE* pipe = *iter;
        pipe->registerbackDependency ( this, TYPE_CELL );
        this->outpipelist->push_back ( pipe );
    }

    this->count = count;
    this->parent = parent;
    this->isrun = true;
    this->isEnd = false;
    this->XIXO = XIXO;
    this->mutex = PTHREAD_MUTEX_INITIALIZER;
    this->condition = PTHREAD_COND_INITIALIZER;
}

void CELL::schedulingStart()
{
    pthread_create ( &this->scheduling_tid, NULL, &CELL::scheduling_wrapper, this );
}

void* CELL::scheduling_wrapper ( void* context )
{
    return ( ( CELL* ) context )->scheduling ( context );
}

list< PIPE* >* CELL::getoutpipelist()
{
    return this->outpipelist;
}

void CELL::schedulerSleep()
{
    int lockerror = pthread_mutex_trylock ( &this->mutex );
    this->isrun = false;
    lockerror = pthread_mutex_unlock ( &this->mutex );
}

void CELL::schedulerWakeup()
{
    int lockerror = pthread_mutex_lock ( &this->mutex );
    this->isrun = true;
    int error = pthread_cond_signal ( &this->condition );
    lockerror = pthread_mutex_unlock ( &this->mutex );
}

bool CELL::getCELLState()
{
    return this->isrun;
}

/*
bool CELL::getWORKERState()
{
    // 워커가 없는 경우 true
    if(workers.size() == 0)
    {
        return false;
    }
    
    bool checker = false;
    int cnt = 0;
    // 워커 검색하여 워커중 하나라도 작동하는지 검사함
    for ( auto iter = workers.begin(); iter != workers.end(); ++iter )
    {
        WORKER* worker = *iter;
        checker |= worker->isWorking();
        if ( worker->isWorking() == true )
        {
            cnt++;
        }
    }

    return checker;
}*/

ushort CELL::getXIXO()
{
    return this->XIXO;
}

STREAMFACTORY* CELL::getParentFactory()
{
    return this->parent;
}

void CELL::setignoreuuid ( unsigned int uuid )
{
    if(this->inpipelist != NULL)
    {
        for(auto iter = this->inpipelist->begin(); iter != this->inpipelist->end(); ++iter)
        {
            PIPE* pipe = *iter;
            pipe->setingnoreuuid(uuid);
        }
    }
}

void CELL::clearignoreuuid()
{
    if(this->inpipelist != NULL)
    {
        for(auto iter = this->inpipelist->begin(); iter != this->inpipelist->end(); ++iter)
        {
            PIPE* pipe = *iter;
            pipe->clearignoreuuid();
        }
    }
}

bool CELL::hasuuid ( unsigned int uuid )
{
    // 워커가 없는 경우 true
    if(workers.size() == 0)
    {
        return false;
    }
    
    bool checker = false;
    // 워커 검색하여 워커중 하나라도 작동하는지 검사함
    for ( auto iter = workers.begin(); iter != workers.end(); ++iter )
    {
        WORKER* worker = *iter;
        checker |= worker->hasuuid(uuid);
    }

    return checker;
}
