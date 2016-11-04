#include "CONNECTION.h"
#include "TUPLE.h"
#include "PIPE.h"
#include "STREAMFACTORY.h"
#include "FACTORYBUILDER.h"
#include "Debug.h"
#include "MIGRATION.h"
#include <errno.h>
#include <string.h>

CONNECTION::CONNECTION ( int recvport, STREAMFACTORY* factory )
{
    int ret = 0;

    _broker_sock = socket ( AF_INET, SOCK_STREAM, IPPROTO_TCP );
    bzero ( &_broker_addr, sizeof ( _broker_addr ) );

    _broker_addr.sin_family = AF_INET;
    _broker_addr.sin_addr.s_addr = INADDR_ANY;
    _broker_addr.sin_port = htons ( recvport );

    ret = ::bind ( _broker_sock, ( struct sockaddr* ) &_broker_addr, sizeof ( _broker_addr ) );
    if ( ret != 0 )
    {
        printf ( "Bind error! %s port: %d\n", strerror(errno), recvport );
        exit ( 0 );
    }
    ret = ::listen ( _broker_sock, 5 );
    if ( ret != 0 )
    {
        printf ( "Listen error!\n" );
        exit ( 0 );
    }
    this->state = false;
    this->shouldbeSleep = false;

    this->factory = factory;

    printf ( "Server initialized!\n" );

}

CONNECTION::~CONNECTION()
{
    close ( this->_broker_sock );
    for ( auto iter = this->clientlists.begin(); iter != this->clientlists.end(); ++iter )
    {
        CLIENT* clnt = *iter;
        close ( clnt->fd );
    }
}

void* CONNECTION::get_in_addr ( sockaddr* sa )
{
    if ( sa->sa_family == AF_INET )
    {
        return & ( ( ( struct sockaddr_in* ) sa )->sin_addr );
    }
    return & ( ( ( struct sockaddr_in6* ) sa )->sin6_addr );
}

pthread_t CONNECTION::serverStart ( PIPE* inpipe, list< PIPE* >* outpipelist )
{
    struct CONNDATA* serverdata = new struct CONNDATA;
    serverdata->thispointer = this;
    serverdata->inpipe = inpipe;
    serverdata->outpipelist = outpipelist;

    // que dependency 설정
    inpipe->registerbackDependency ( this, TYPE_CONNECTION );
    for ( auto iter = outpipelist->begin(); iter != outpipelist->end(); ++iter )
    {
        auto pipe = *iter;
        pipe->registerforwardDependency ( this, TYPE_CONNECTION );
    }

    pthread_t server_tid;
    pthread_create ( &server_tid, NULL, &CONNECTION::serverStart_wrapper, ( void* ) serverdata );

    return server_tid;
}

void* CONNECTION::serverStart_wrapper ( void* context )
{
    struct CONNDATA* conndata = ( struct CONNDATA* ) context;
    void* thispointer = conndata->thispointer;
    return ( ( CONNECTION* ) thispointer )->serverStart_internal ( context );
}

void* CONNECTION::serverStart_internal ( void* arg )
{
    struct CONNDATA* serverdata = ( struct CONNDATA* ) arg;
    PIPE* inpipe = serverdata->inpipe;
    list<PIPE*>* outpipelist = serverdata->outpipelist;

    // Dispatcher Install
    struct CONNDATA* dispatcherdata = new struct CONNDATA;
    dispatcherdata->fd = 0;
    dispatcherdata->inpipe = inpipe;
    dispatcherdata->dispatchpipe = new PIPE ( new lockfreeq ( 0 ), NULL, 2, 0 );
    dispatcherdata->thispointer = this;
    pthread_create ( &this->dispatcher_tid, NULL, &CONNECTION::dispatcher_wrapper, ( void* ) dispatcherdata );

    
    // Sender Install
    pthread_t sender_tid;
    struct CONNDATA* senderdata = new struct CONNDATA;
    senderdata->fd = 0;
    senderdata->outpipelist = outpipelist;
    senderdata->thispointer = this;
    pthread_create ( &this->sender_tid, NULL, &CONNECTION::sender_wrapper, ( void* ) senderdata );

    while ( 1 )
    {
        struct sockaddr_storage client_addr;
        char address[20];
        socklen_t sin_size = sizeof client_addr;
        pthread_t id;

        printf ( "Ready to accept clients!\n" );

        int fd = accept ( _broker_sock, ( struct sockaddr * ) &client_addr, &sin_size );
        if ( fd == -1 )
        {
            printf ( "Accept error!\n" );
            continue;
        }

        inet_ntop ( client_addr.ss_family, get_in_addr ( ( struct sockaddr * ) &client_addr ), address, sizeof address );

        printf ( "[NOTICE] Got connection from %s\n", address );

        // STATE 변경 to TRUE
        this->state = true;

        struct CONNDATA* receiverdata = new struct CONNDATA;
        receiverdata->fd = fd;
        receiverdata->dispatchpipe = dispatcherdata->dispatchpipe;
        receiverdata->thispointer = this;

        pthread_t receiver_tid;
        pthread_create ( &receiver_tid, NULL, &CONNECTION::receiver_wrapper, ( void* ) receiverdata );
        this->receiver_tids.push_back ( receiver_tid );
    }

    return NULL;
}

void* CONNECTION::receiver ( void* arg )
{
    struct CONNDATA* conndata = ( struct CONNDATA* ) arg;
    lockfreeq* dispatchq = conndata->dispatchpipe->getQueue();
    char buffer[4096] = "";

    while ( 1 )
    {
        if ( this->shouldbeSleep == true )
        {
            printf ( "receiver goes to sleep\n" );
            pthread_cond_wait ( &this->condition, &this->mutex );
            printf ( "receiver goes to wakeup\n" );
        }

        // 데이터를 받는다.
        unsigned int size = recv ( conndata->fd, buffer, sizeof buffer, 0 );
        if ( ( size == -1 ) || ( size == 0 ) )
            break;

        // 데이터를 DATA 구조체로 바꾼다.
        char* pdata = new char[size];
        memset ( pdata, 0, size );
        memcpy ( pdata, buffer, size );

        TUPLE* data = new TUPLE ( pdata, size - 4, 0, conndata->fd );

        // DATA 구조체를 넣는다.
        if ( !dispatchq->push ( data ) )
        {
            printf ( "fail to receive (Push error!)\n" );
            exit ( 0 );
        }

        // 내부 변수 초기화
        memset ( buffer, 0, sizeof buffer );
    }
    //printf ( "Error occured in receiverr or exited\n" );
    return NULL;
}

void* CONNECTION::receiver_wrapper ( void* context )
{
    struct CONNDATA* conndata = ( struct CONNDATA* ) context;
    void* thispointer = conndata->thispointer;
    void* result = ( ( CONNECTION* ) thispointer )->receiver ( context );

    return result;
}

void* CONNECTION::sender ( void* arg )
{
    struct CONNDATA* conndata = ( struct CONNDATA* ) arg;
    auto outpipelist = conndata->outpipelist;
    //lockfreeq* outq = conndata->outq->getQueue();

    while ( 1 )
    {
        if ( this->shouldbeSleep == true )
        {
            printf ( "sender goes to sleep\n" );
            pthread_cond_wait ( &this->condition, &this->mutex );
            printf ( "sender goes to wakeup\n" );
        }

        // outq 모두에 대해 검사 및 전송

        for ( auto iter = outpipelist->begin(); iter != outpipelist->end(); ++iter )
        {
            lockfreeq* outq = ( *iter )->getQueue();
            while ( !outq->empty() )
            {
                TUPLE* tuple;
                if ( !outq->pop ( tuple ) )
                {
                    printf ( "outq pop error!\n" );
                    exit ( 0 );
                }

                //debug_packet(data->getdata(), data->getLen() + 4);

                if ( tuple->getcontent() == NULL )
                {
                    printf ( "No contents!\n" );
                    exit ( 0 );
                }

                /*if ( send ( data->getfd(), data->getdata(), data->getLen(), 0 ) == -1 )
                {
                    printf ( "Error occured in send function!\n" );
                    while(1);
                    exit ( 0 );
                }*/

                delete[] tuple->getdata();
                delete tuple;
            }

        }
    }
}

void* CONNECTION::sender_wrapper ( void* context )
{
    struct CONNDATA* conndata = ( struct CONNDATA* ) context;
    void* thispointer = conndata->thispointer;
    return ( ( CONNECTION* ) thispointer )->sender ( context );
}

void* CONNECTION::dispatcher ( void* arg )
{
    struct CONNDATA* conndata = ( struct CONNDATA* ) arg;
    lockfreeq* dispatchq = conndata->dispatchpipe->getQueue();
    lockfreeq* inq = conndata->inpipe->getQueue();
    while ( 1 )
    {
        if ( !dispatchq->empty() )
        {
            TUPLE* tuple;
            dispatchq->pop ( tuple );

            // packet validity check
            if ( !tuple->validity() )
            {
                printf ( "data is not valid in dispatch, size: %d\n", tuple->getLen() );
                continue;
            }

            switch ( tuple->gettype() )
            {
                // 데이터
            case 0x00:
            {
                // inq에 넣음
                inq->push ( tuple );
                break;
            }
            // 마이그레이션 실행
            case 0x01:
            {
                MIGRATION* mig = factorymanager.getMIGRATION();
                // 0번 아이디에 executor에 해당하는곳에 마이그레이션 시작
                mig->startMIGRATION(0, this->factory);
                break;
            }
            default:
            {
                printf ( "Invalid type data in dispatcher!\n" );
                break;
            }
            }
        }
    }
}

void* CONNECTION::dispatcher_wrapper ( void* context )
{
    struct CONNDATA* conndata = ( struct CONNDATA* ) context;
    void* thispointer = conndata->thispointer;
    return ( ( CONNECTION* ) thispointer )->dispatcher ( context );
}

/*
void Connection::register_user(pthread_t send_tid, pthread_t recv_tid, int fd, struct sockaddr_storage client_addr)
{
    struct CLIENT* client = new struct CLIENT;
    struct sockaddr_storage* pclient_addr = new struct sockaddr_storage;
    memcpy(pclient_addr, &client_addr, sizeof(sockaddr_storage));

    client->fd = fd;
    client->send_tid = send_tid;
    client->recv_tid = recv_tid;
    client->sockaddr = pclient_addr;

    clientlists.push_back(client);
}*/

void CONNECTION::setsender_tid ( pthread_t tid )
{
    this->sender_tid = tid;
}

void CONNECTION::setdispatcher_tid ( pthread_t tid )
{
    this->dispatcher_tid = tid;
}

bool CONNECTION::getConnState()
{
    return this->state;
}

void CONNECTION::sleepCONNECTOR()
{
    pthread_mutex_lock ( &this->mutex );
    this->shouldbeSleep = true;
    pthread_mutex_unlock ( &this->mutex );
}

void CONNECTION::wakeupCONNECTOR()
{
    pthread_mutex_lock ( &this->mutex );
    this->shouldbeSleep = false;
    pthread_cond_broadcast ( &this->condition );
    pthread_mutex_unlock ( &this->mutex );
}
