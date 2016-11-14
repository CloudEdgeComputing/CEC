#include "SRCCELL.h"
#include "STREAMFACTORY.h"
#include "PIPE.h"
#include "Type.h"
#include "PACKET.h"
#include "TUPLE.h"
#include "Debug.h"
#include <FACTORYBUILDER.h>

SRCCELL::SRCCELL ( int recvport, list< PIPE* >* outpipelist, STREAMFACTORY* parent ) : CELL ( outpipelist, 1, parent, SO )
{
    this->recvport = recvport;
}

SRCCELL::SRCCELL ( int recvport, PIPE* outpipe, STREAMFACTORY* parent ) : CELL ( outpipe, 1, parent, SO )
{
    this->recvport = recvport;
}

void SRCCELL::makeWorker()
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
    this->isrun = false;
    this->isEnd = false;

    printf ( "SRCCELL initialized!\n" );
}

void* SRCCELL::scheduling ( void* arg )
{
    struct CONNDATA* serverdata = new struct CONNDATA;

    serverdata->thispointer = this;
    serverdata->outpipelist = this->outpipelist;

    pthread_t SRCBROKER_tid;
    pthread_create ( &SRCBROKER_tid, NULL, &SRCCELL::SRCbroker_start_wrapper, ( void* ) serverdata );
}

void* SRCCELL::SRCbroker_start_wrapper ( void* context )
{
    struct CONNDATA* conndata = ( struct CONNDATA* ) context;
    void* thispointer = conndata->thispointer;
    return ( ( SRCCELL* ) thispointer )->SRCbroker_start_internal ( context );
}

void* SRCCELL::SRCbroker_start_internal ( void* context )
{
    struct CONNDATA* srcctx = ( struct CONNDATA* ) context;

    while ( 1 )
    {
        struct sockaddr* client_addr = new struct sockaddr;
        socklen_t sin_size = sizeof client_addr;
        
        //printf ( "ready to accept clients from srccell!\n" );
        int fd = accept ( this->_broker_sock, client_addr, &sin_size );
        if ( fd == -1 )
        {
            printf ( "accept error in %s\n", __func__ );
            continue;
        }
        
        printf("accepted!\n");
        
        this->isrun = true;
        
        // user register
        CLIENT* client = new CLIENT;
        client->fd = fd;
        client->var_sockaddr = client_addr;
        this->registerDevice(client);

        struct CONNDATA* receiverdata = new struct CONNDATA;
        receiverdata->fd = fd;
        receiverdata->thispointer = this;
        receiverdata->outpipelist = this->outpipelist;
        receiverdata->client = client;

        pthread_t receiver_tid;
        pthread_create ( &receiver_tid, NULL, &SRCCELL::SRCrecv_start_wrapper, ( void* ) receiverdata );
    }
}

void* SRCCELL::SRCrecv_start_wrapper ( void* context )
{
    struct CONNDATA* conndata = ( struct CONNDATA* ) context;
    void* thispointer = conndata->thispointer;
    return ( ( SRCCELL* ) thispointer )->SRCrecv_start_internal ( context );
}

void* SRCCELL::SRCrecv_start_internal ( void* context )
{
    struct CONNDATA* srcctx = ( struct CONNDATA* ) context;

    char buffer[4096] = "";

    // 패킷 어셈블러에 쓰이는 자료형
    deque<PACKET*>* packetque = new deque<PACKET*>;
    unsigned char* seq = new unsigned char;
    *seq = 0;

    while ( 1 )
    {
        unsigned int size = recv ( srcctx->fd, buffer, sizeof ( buffer ), 0 );
        printf("size: %d\n", size);
        if ( ( size == -1 ) || ( size == 0 ) )
            break;

        // 패킷 어셈블 함수 호출
        
        this->pushPacket(buffer, size, packetque, seq);
        
        while ( 1 )
        {
            int outsize = 0;
            
            // 패킷 어셈블된것 받아오기
            char* result = this->getPacketassembled(&outsize, packetque);
            
            if ( result == NULL )
            {
                break;
            }

            // DATA 구조체를 넣는다. 이때 튜플은 독립적으로 흘러가야 하므로 새로 생성해서 넘겨준다.
            for ( auto iter = srcctx->outpipelist->begin(); iter != srcctx->outpipelist->end(); ++iter )
            {
                PIPE* pipe = *iter;
                TUPLE* data = new TUPLE ( result, outsize - 4, 0, 0 );
                
                // 어셈블 된 튜플의 타입이 0x01인경우 startmigration 실행
                if(data->gettype() == 0x01)
                {
                    this->parent->startMIGRATION(srcctx->client->uuid);
                    delete[] data->getdata();
                    delete data;
                    continue;
                }
                // 디바이스 등록
                else if(data->gettype() == 0x03)
                {
                    int uuid = data->getInt();
                    srcctx->client->uuid = uuid;
                    printf("registerd client's uuid: %d\n", uuid);
                    delete[] data->getdata();
                    delete data;
                    continue;
                }
                data->setuuid(srcctx->client->uuid);
                pipe->push ( data );
            }
        }
        memset ( buffer, 0, sizeof buffer );
    }
    printf ( "disconnected from device!\n" );
    return NULL;
}

void SRCCELL::pushPacket ( char* input, int size, deque< PACKET* >* packetque, unsigned char* seq )
{
    PACKET* pkt = new PACKET ( input, size, *seq++ );

    packetque->push_back ( pkt );

    if ( packetque->size() > 5 )
    {
        printf ( "packet has loss\n" );
        // 패킷 로스 날 경우 패킷을 앞에서 모두 훍어 0xaa를 찾은 뒤, 사이즈를 검사 해봐야 한다. 아마 n^2 알고리즘일듯
        exit ( 0 );
    }
}

char* SRCCELL::getPacketassembled ( int* size, deque< PACKET* >* packetque )
{

    vector<char> byte_array;
    byte_array.clear();
    deque<PACKET*> candidate;
    candidate.clear();

    if ( packetque->size() == 0 )
    {
        *size = 0;
        return NULL;
    }

    PACKET* firstpkt = packetque->at ( 0 );

    char* byte = firstpkt->getArray();

    if ( ( unsigned char ) *byte != 0xAA )
    {
        *size = 0;
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
            delete[] pkt->getArray();
            delete pkt;
            break;
        }
        else
        {
            iter = candidate.erase ( iter );
            delete[] pkt->getArray();
            delete pkt;
        }
    }

    *size = byte_array.size();

    char* result = new char[*size];

    memcpy ( result, byte_array.data(), byte_array.size() );

    return result;
}

void SRCCELL::registerDevice ( CLIENT* client )
{
    // 지역 가입자 (셀 내의 관리구조체)
    this->devices.push_back(client);
    
    // 글로벌 가입자 (팩토리 내의 관리 구조체)
    auto globaldevice = this->parent->getsrcdevices();
    globaldevice->push_back(client);
}