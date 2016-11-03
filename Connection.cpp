#include "Connection.h"
#include "Data.h"
#include "Queue.h"
#include "Executor.h"
#include "ExecutorManager.h"
#include "Debug.h"
#include "Migration.h"

Connection::Connection ( int recvport, Executor* executor )
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
        printf ( "Bind error!\n" );
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

    this->executor = executor;

    printf ( "Server initialized!\n" );

}

Connection::~Connection()
{
    close ( this->_broker_sock );
    for ( auto iter = this->clientlists.begin(); iter != this->clientlists.end(); ++iter )
    {
        CLIENT* clnt = *iter;
        close ( clnt->fd );
    }
}

void* Connection::get_in_addr ( sockaddr* sa )
{
    if ( sa->sa_family == AF_INET )
    {
        return & ( ( ( struct sockaddr_in* ) sa )->sin_addr );
    }
    return & ( ( ( struct sockaddr_in6* ) sa )->sin6_addr );
}

pthread_t Connection::serverStart ( QUEUE* inq, list<QUEUE*>* outqlist )
{
    struct CONNDATA* serverdata = new struct CONNDATA;
    serverdata->thispointer = this;
    serverdata->inq = inq;
    serverdata->outqlist = outqlist;
    
    // que dependency 설정
    inq->registerbackDependency(this, TYPE_CONNECTION);
    for(auto iter = outqlist->begin(); iter != outqlist->end(); ++iter)
    {
        auto que = *iter;
        que->registerforwardDependency(this, TYPE_CONNECTION);
    }
    
    pthread_t server_tid;
    pthread_create ( &server_tid, NULL, &Connection::serverStart_wrapper, ( void* ) serverdata );

    return server_tid;
}

void* Connection::serverStart_wrapper ( void* context )
{
    struct CONNDATA* conndata = ( struct CONNDATA* ) context;
    void* thispointer = conndata->thispointer;
    return ( ( Connection* ) thispointer )->serverStart_internal ( context );
}

void* Connection::serverStart_internal ( void* arg )
{
    struct CONNDATA* serverdata = ( struct CONNDATA* ) arg;
    QUEUE* inq = serverdata->inq;
    list<QUEUE*>* outqlist = serverdata->outqlist;

    // Dispatcher Install
    struct CONNDATA* dispatcherdata = new struct CONNDATA;
    dispatcherdata->fd = 0;
    dispatcherdata->inq = inq;
    dispatcherdata->dispatchq = new QUEUE ( new lockfreeq ( 0 ), NULL, 2, 0 );
    dispatcherdata->thispointer = this;
    pthread_create ( &this->dispatcher_tid, NULL, &Connection::dispatcher_wrapper, ( void* ) dispatcherdata );

    // Sender Install
    pthread_t sender_tid;
    struct CONNDATA* senderdata = new struct CONNDATA;
    senderdata->fd = 0;
    senderdata->outqlist = outqlist;
    senderdata->thispointer = this;
    pthread_create ( &this->sender_tid, NULL, &Connection::sender_wrapper, ( void* ) senderdata );

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
        //receiverdata->inq = inq;
        //receiverdata->outq = outq;
        receiverdata->dispatchq = dispatcherdata->dispatchq;
        receiverdata->thispointer = this;

        pthread_t receiver_tid;
        pthread_create ( &receiver_tid, NULL, &Connection::receiver_wrapper, ( void* ) receiverdata );
        this->receiver_tids.push_back ( receiver_tid );
    }

    return NULL;
}

void* Connection::receiver ( void* arg )
{
    struct CONNDATA* conndata = ( struct CONNDATA* ) arg;
    lockfreeq* dispatchq = conndata->dispatchq->getQueue();
    char buffer[4096] = "";

    while ( 1 )
    {
        if ( this->shouldbeSleep == true )
        {
            printf ( "receiver goes to sleep\n" );
            pthread_cond_wait ( &this->g_condition, &this->g_mutex );
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

        DATA* data = new DATA ( pdata, size - 4, 0, conndata->fd );

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

void* Connection::receiver_wrapper ( void* context )
{
    struct CONNDATA* conndata = ( struct CONNDATA* ) context;
    void* thispointer = conndata->thispointer;
    void* result = ( ( Connection* ) thispointer )->receiver ( context );

    return result;
}

void* Connection::sender ( void* arg )
{
    struct CONNDATA* conndata = ( struct CONNDATA* ) arg;
    auto outqlist = conndata->outqlist;
    //lockfreeq* outq = conndata->outq->getQueue();

    while ( 1 )
    {
        if ( this->shouldbeSleep == true )
        {
            printf ( "sender goes to sleep\n" );
            pthread_cond_wait ( &this->g_condition, &this->g_mutex );
            printf ( "sender goes to wakeup\n" );
        }
        
        // outq 모두에 대해 검사 및 전송
        
        for ( auto iter = outqlist->begin(); iter != outqlist->end(); ++iter )
        {
            lockfreeq* outq = (*iter)->getQueue();
            while ( !outq->empty() )
            {
                DATA* data;
                if ( !outq->pop ( data ) )
                {
                    printf ( "outq pop error!\n" );
                    exit ( 0 );
                }

                //debug_packet(data->getdata(), data->getLen() + 4);

                if ( data->getcontent() == NULL )
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

                delete[] data->getdata();
                delete data;
            }

        }
    }
}

void* Connection::sender_wrapper ( void* context )
{
    struct CONNDATA* conndata = ( struct CONNDATA* ) context;
    void* thispointer = conndata->thispointer;
    return ( ( Connection* ) thispointer )->sender ( context );
}

void* Connection::dispatcher ( void* arg )
{
    struct CONNDATA* conndata = ( struct CONNDATA* ) arg;
    lockfreeq* dispatchq = conndata->dispatchq->getQueue();
    lockfreeq* inq = conndata->inq->getQueue();
    while ( 1 )
    {
        if ( !dispatchq->empty() )
        {
            DATA* data;
            dispatchq->pop ( data );

            // packet validity check
            if ( !data->validity() )
            {
                printf ( "data is not valid in dispatch, size: %d\n", data->getLen() );
                continue;
            }

            switch ( data->gettype() )
            {
                // 데이터
            case 0x00:
            {
                // inq에 넣음
                inq->push ( data );
                break;
            }
            // 마이그레이션 실행
            case 0x01:
            {
                Migration* mig = executormanager.getMigrationModule();
                // 0번 아이디에 executor에 해당하는곳에 마이그레이션 시작
                mig->startMigration ( 0, this->executor );
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

void* Connection::dispatcher_wrapper ( void* context )
{
    struct CONNDATA* conndata = ( struct CONNDATA* ) context;
    void* thispointer = conndata->thispointer;
    return ( ( Connection* ) thispointer )->dispatcher ( context );
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

void Connection::setsender_tid ( pthread_t tid )
{
    this->sender_tid = tid;
}

void Connection::setdispatcher_tid ( pthread_t tid )
{
    this->dispatcher_tid = tid;
}

bool Connection::getConnState()
{
    return this->state;
}

void Connection::sleepConnection()
{
    pthread_mutex_lock ( &this->g_mutex );
    this->shouldbeSleep = true;
    pthread_mutex_unlock ( &this->g_mutex );
}

void Connection::wakeupConnection()
{
    pthread_mutex_lock ( &this->g_mutex );
    this->shouldbeSleep = false;
    pthread_cond_broadcast ( &this->g_condition );
    pthread_mutex_unlock ( &this->g_mutex );
}
