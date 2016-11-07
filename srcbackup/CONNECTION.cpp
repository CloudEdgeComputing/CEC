#include "CONNECTION.h"
#include "TUPLE.h"
#include "PIPE.h"
#include "STREAMFACTORY.h"
#include "FACTORYBUILDER.h"
#include "Debug.h"
#include "MIGRATION.h"
#include "PACKET.h"
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
        printf ( "Bind error! %s port: %d\n", strerror ( errno ), recvport );
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
    dispatcherdata->dispatchpipe = new PIPE ( new lockfreeq ( 0 ), NULL, 2, 0, NULL );
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
        struct sockaddr* client_addr = new struct sockaddr;
        char address[20];
        socklen_t sin_size = sizeof client_addr;
        pthread_t id;

        printf ( "Ready to accept clients!\n" );

        int fd = accept ( _broker_sock, client_addr, &sin_size );
        if ( fd == -1 )
        {
            printf ( "Accept error!\n" );
            continue;
        }

        //inet_ntop ( client_addr->ss_family, get_in_addr, client_addr, address, sizeof address );

        //printf ( "[NOTICE] Got connection from %s\n", address );

        // STATE 변경 to TRUE
        this->state = true;

        struct CONNDATA* receiverdata = new struct CONNDATA;
        receiverdata->fd = fd;
        receiverdata->dispatchpipe = dispatcherdata->dispatchpipe;
        receiverdata->thispointer = this;

        pthread_t receiver_tid;
        pthread_create ( &receiver_tid, NULL, &CONNECTION::receiver_wrapper, ( void* ) receiverdata );
        this->register_user ( fd, client_addr );
        this->receiver_tids.push_back ( receiver_tid );
    }

    return NULL;
}

// 각자 개인당 생성되는 리시버
void* CONNECTION::receiver ( void* arg )
{
    struct CONNDATA* conndata = ( struct CONNDATA* ) arg;
    lockfreeq* dispatchq = conndata->dispatchpipe->getQueue();
    char buffer[4096] = "";

    // 패킷 어셈블, 개인당 생성
    deque<PACKET*>* packetque = new deque<PACKET*>;
    unsigned char* seq = new unsigned char;
    *seq = 0;

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

        // fragmented data를 조립한다.
        // packet 저장은 (seq(char), bytearray..

        this->assemblePacket ( buffer, size, packetque, seq );

        while ( 1 )
        {
            uint outsize = 0;
            char* result = this->getassembledPacket ( &outsize, packetque );

            if ( result == NULL )
            {
                printf ( "All flushed or not full packet!\n" );
                break;
            }

            TUPLE* data = new TUPLE ( result, outsize - 4, 0, conndata->fd );

            // DATA 구조체를 넣는다.
            if ( !dispatchq->push ( data ) )
            {
                printf ( "fail to receive (Push error!)\n" );
                exit ( 0 );
            }
        }

        // 내부 변수 초기화
        memset ( buffer, 0, sizeof buffer );
    }
    printf ( "disconnected!\n" );
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
                                
                if ( send ( tuple->getfd(), tuple->getdata(), tuple->getLen() + 4, 0 ) == -1 )
                {
                    printf ( "Error occured in send function!\n" );
                    while(1);
                    exit ( 0 );
                }

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
                mig->startMIGRATION ( 0, this->factory );
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

void CONNECTION::register_user ( int fd, sockaddr* client_addr )
{
    struct CLIENT* client = new struct CLIENT;
    
    client->fd = fd;
    client->var_sockaddr = client_addr;
    
    clientlists.push_back ( client );
}

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

// TODO
void CONNECTION::assemblePacket ( char* input, uint insize, deque< PACKET* >* packetque, unsigned char* seq )
{
    PACKET* pkt = new PACKET ( input, insize, *seq++ );

    packetque->push_back ( pkt );

    if ( packetque->size() > 5 )
    {
        printf ( "packet has loss\n" );
        // 패킷 로스 날 경우 패킷을 앞에서 모두 훍어 0xaa를 찾은 뒤, 사이즈를 검사 해봐야 한다. 아마 n^2 알고리즘일듯
        exit ( 0 );
    }
}

char* CONNECTION::getassembledPacket ( uint* outsize, deque< PACKET* >* packetque )
{
    vector<char> byte_array;
    deque<PACKET*> candidate;

    if ( packetque->size() == 0 )
    {
        *outsize = 0;
        return NULL;
    }

    PACKET* firstpkt = packetque->at ( 0 );

    char* byte = firstpkt->getArray();

    if ( ( unsigned char ) *byte != 0xAA )
    {
        *outsize = 0;
        return NULL;
    }

    unsigned short expectedsize = 0;
    memcpy ( &expectedsize, &byte[1], 2 );

    expectedsize += 4;


    candidate.clear();

    unsigned int accumulatedsize = 0;

    // 예상 패킷 사이즈만큼 모든 패킷 청크를 다 꺼내 candidate로 보냄
    while ( accumulatedsize < expectedsize )
    {
        PACKET* pkt = packetque->front();
        candidate.push_back ( pkt );
        accumulatedsize += pkt->getSize();
        packetque->pop_front();
        //printf("accumulatedsize: %d expectedsize: %d\n", accumulatedsize, expectedsize);
    }

    accumulatedsize = 0;

    for ( auto iter = candidate.begin(); iter != candidate.end(); )
    {
        PACKET* pkt = *iter;
        // expected size만큼 꺼냄
        accumulatedsize += pkt->getSize();

        unsigned int min_size = accumulatedsize <= expectedsize ? accumulatedsize : expectedsize;

        byte_array.insert ( byte_array.end(), pkt->getArray(), pkt->getArray() + min_size );

        if ( accumulatedsize > expectedsize )
        {
            // 다시 packetque에 넣어야 함.. 하지만 min_size는 이미 나갔으므로 그만큼의 패킷을 빼줘야 함
            char* array = pkt->getArray();
            char* new_array = new char[ pkt->getSize() - min_size ];
            memcpy ( new_array, &array[min_size], pkt->getSize() - min_size );
            pkt->setArray ( new_array );
            pkt->setSize ( pkt->getSize() - min_size );
            packetque->push_front ( pkt );
            delete[] array;
            break;
        }
        else if ( accumulatedsize == expectedsize )
        {
            iter = candidate.erase ( iter );
            break;
        }
        else
        {
            iter = candidate.erase ( iter );
        }
    }

    *outsize = byte_array.size();

    char* result = new char[*outsize];

    memcpy ( result, byte_array.data(), byte_array.size() );

    return result;
}

unsigned int CONNECTION::getipbyfd(int fd)
{
    for ( auto iter = this->clientlists.begin(); iter != this->clientlists.end(); ++iter )
    {
        CLIENT* client = *iter;
        struct sockaddr_in* sain= (sockaddr_in*)client->var_sockaddr;
        if(fd == client->fd)
            return sain->sin_addr.s_addr;
    }
    printf("Nothing found..%s\n", __func__);
    return 0;
}

unsigned int CONNECTION::getfdbyip ( int ip )
{
    for ( auto iter = this->clientlists.begin(); iter != this->clientlists.end(); ++iter )
    {
        CLIENT* client = *iter;
        struct sockaddr_in* sain= (sockaddr_in*)client->var_sockaddr;
        int _ip = sain->sin_addr.s_addr;
        if( ip == _ip )
            return client->fd;
    }
    printf("Nothing found..%s\n", __func__);
    return 0;
}