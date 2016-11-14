#include "MIGRATION.h"
#include "STREAMFACTORY.h"
#include "PIPE.h"
#include "BASICCELL.h"
#include "FACTORYBUILDER.h"
#include "TUPLE.h"
#include "PACKET.h"
#include "Debug.h"
#include <STATEMANAGER.h>

MIGRATION::MIGRATION ( int port, char comm )
{
    // 접속을 기다릴 수 있는 서버를 만듦
    if ( comm == 's' )
    {
        this->makeServer ( port );
    }
    // 서버에 접속함
    else if ( comm == 'c' )
    {
        // Get MIGRATION server list
        list<SERVER*> _list = this->getnearserver();

        // 인접한 서버 리스트에 대하여 커넥션 생성 (내가 서버인가 클라이언트인가를 지정)
        for ( auto iter = _list.begin(); iter != _list.end(); ++iter )
        {
            SERVER* serv = *iter;
            int fd = this->makeConnection ( serv->ip, serv->port, serv->isInstalled );
        }
    }

    // MIGRATION 관한 데이터를 받을 쓰레드를 생성함
    // Data migration을 일으키는 데이터는 Connection 쓰레드에서 알아서 함
    this->waitforMigration();
}

list< SERVER* > MIGRATION::getnearserver()
{
    // 현재 하드코딩 TODO
    // 원래 설계는 전체 엣지 노드들을 관리하는 파라미터 서버같은것이 있고 거기에서 받아와야 함

    SERVER* server = new SERVER;
    server->ip = INADDR_ANY;
    server->port = 1200;
    server->isInstalled = true;
    this->nearserver.push_back ( server );

    return this->nearserver;
}

int MIGRATION::makeConnection ( unsigned int ip, ushort port, bool isInstalled )
{
    if ( isInstalled == true )
    {
        struct sockaddr_in serv_addr;
        int sockfd = socket ( AF_INET, SOCK_STREAM, 0 );

        serv_addr.sin_family = AF_INET;
        serv_addr.sin_port = htons ( port );
        serv_addr.sin_addr.s_addr = ip;

        int result = connect ( sockfd, ( struct sockaddr* ) &serv_addr, sizeof ( serv_addr ) );
        if ( result != 0 )
        {
            printf ( "connection failed to CEC\n" );
            exit ( 0 );
        }

        printf ( "connecting fd: %d\n", sockfd );
        this->registerServer ( sockfd );
    }
}

int MIGRATION::makeServer ( ushort port )
{
    this->server_port = port;

    pthread_t tid;
    pthread_create ( &tid, NULL, internal_makeServer, ( void* ) this );
    // nLen은 전체 사이즈가 들어가야 함

}

void MIGRATION::startMIGRATION ( unsigned int uuid, int cec_id, STREAMFACTORY* factory )
{
    int sockfd = this->getClienctCEC ( cec_id );

    // executor에 해당하는 모든 task들을 블럭시킨다.
    // Task scheduler task 블럭, 웨이크업

    auto list = factory->getCELLs();
    
    printf("migration start!\n");
    
    
    printf("blocking all tuples using uuid: %d\n", uuid);
    // cell에 specific uuid가 못들어가도록 설정
    for ( auto iter = list.begin(); iter != list.end(); ++iter )
    {
        CELL* cell = ( *iter );
        cell->setignoreuuid(uuid);
    }

    // 블럭 되어있으니 이제 큐에 있는 데이터들을 전송시도한다.
    while ( 1 )
    {
        bool checker = true;

        auto list = factory->getCELLs();

        //printf ( "from cells\n" );
        for ( auto iter = list.begin(); iter != list.end(); ++iter )
        {
            CELL* cell = *iter;
            // 셀에 있는 아웃풋 파이프 리스트
            auto outpipelist = cell->getoutpipelist();
            
            // DESTCELL의 경우 아웃풋 리스트가 없을 수 있음
            if(outpipelist == NULL)
            {
                continue;
            }
            
            int cnt = 0 ;
            for ( auto iter = outpipelist->begin(); iter != outpipelist->end(); ++iter )
            {
                auto outpipe = *iter;
                checker &= outpipe->sendQueue ( sockfd, uuid );
                usleep ( 500 );
            }
        }

        if ( checker == true )
        {

            for ( auto iter = list.begin(); iter != list.end(); ++iter )
            {
                CELL* cell = *iter;
                
                auto outpipelist = cell->getoutpipelist();
                
                // DESTCELL의 경우 아웃풋 리스트가 없을 수 있음
                if(outpipelist == NULL)
                {
                    continue;
                }
                
                for ( auto iter = outpipelist->begin(); iter != outpipelist->end(); ++iter )
                {
                    auto outpipe = *iter;
                    outpipe->clearMigration();
                }
                cell->clearignoreuuid();
            }
            break;
        }
    }
    
    // DB migration
    STATEMANAGER* statemanager = factory->getStatemanager();
    statemanager->migrateDB(sockfd, uuid);

    printf ( "MIGRATION completed!\n" );
}

void MIGRATION::waitforMigration()
{
    pthread_t tid;
    pthread_create ( &tid, NULL, internal_waitforMIGRATION, ( void* ) this );
}

int MIGRATION::getClienctCEC ( int n )
{
    printf ( "wait for connecting...!\n" );
    while ( 1 )
    {
        int cnt = 0;
        for ( auto iter = this->connected_clients.begin(); iter != this->connected_clients.end(); ++iter )
        {
            if ( cnt == n )
            {
                printf ( "got client cec fd!\n" );
                return *iter;
            }
            else
                cnt++;
        }
    }

    return 0;
}

void MIGRATION::registerClient ( int fd )
{
    this->connected_clients.push_back ( fd );
}

void MIGRATION::registerServer ( int fd )
{
    this->connecting_servers.push_back ( fd );
}

bool MIGRATION::getisInstalled()
{
    return this->isInstalled;
}

void MIGRATION::setisInstalled ( bool sw )
{
    this->isInstalled = sw;
}

ushort MIGRATION::getport()
{
    return this->server_port;
}

int MIGRATION::getserver_fd()
{
    return this->server_fd;
}

int MIGRATION::getServerCEC ( int n )
{
    printf ( "searching servers conntected with me!\n" );
    while ( 1 )
    {
        int cnt = 0;
        for ( auto iter = this->connecting_servers.begin(); iter != this->connecting_servers.end(); ++iter )
        {
            if ( cnt == n )
            {
                printf ( "Found server cec fd!\n" );
                return *iter;
            }
            else
                cnt++;
        }
    }

    return 0;
}

void* internal_makeServer ( void* context )
{
    MIGRATION* mig = ( MIGRATION* ) context;

    int ret = 0;

    int sfd = socket ( AF_INET, SOCK_STREAM, 0 );
    sockaddr_in addr;
    bzero ( &addr, sizeof ( addr ) );

    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons ( mig->getport() );

    int nSockOpt = 1;
    setsockopt ( sfd, SOL_SOCKET, SO_REUSEADDR, &nSockOpt, sizeof ( nSockOpt ) );

    ret = ::bind ( sfd, ( struct sockaddr* ) &addr, sizeof ( addr ) );
    if ( ret != 0 )
    {
        printf ( "bind error in MIGRATION server!\n" );
        exit ( 0 );
    }

    ret = ::listen ( sfd, 5 );
    if ( ret != 0 )
    {
        printf ( "listen error in MIGRATION server!\n" );
        exit ( 0 );
    }
    printf ( "MIGRATION server initialized ip : %d port: %d!\n", addr.sin_addr.s_addr, mig->getport() );

    while ( 1 )
    {
        struct sockaddr_storage client_addr;
        unsigned int sin_size;
        int client_fd = accept ( sfd, ( struct sockaddr * ) &client_addr, &sin_size );
        printf ( "Another edge was created and connected to me! %d\n", client_fd );
        mig->registerClient ( client_fd );
    }
}

void* internal_waitforMIGRATION ( void* context )
{
    char buffer[60000] = "";
    MIGRATION* migration = ( MIGRATION* ) context;

    // 여기는 0번째인걸 아니까.. 그냥 하드코딩..
    // 첫번째 등록된 엣지노드의 파일디스크립터
    // 제대로 하려면 select를 이용하여 입력다중화 시켜야 함
    // TODO
    int fd = migration->getServerCEC ( 0 );

    // 패킷 어셈블, 연결된 서버 CEC당 생성
    deque<PACKET*>* packetque = new deque<PACKET*>;
    unsigned char* seq = new unsigned char;
    *seq = 0;

    while ( 1 )
    {
        // 데이터를 받는다.
        unsigned int size = recv ( fd, buffer, sizeof buffer, 0 );
        //printf ( "migration data is received size: %d\n", size );
        if ( ( size == -1 ) || ( size == 0 ) )
            break;

        // 받은 데이터를 쌓는다.
        assemblePacket ( buffer, size, packetque, seq );

        // 받은 데이터를 잘라 받는다.
        while ( 1 )
        {
            uint outsize = 0;
            char* result = getassembledPacket ( &outsize, packetque );

            if ( result == NULL )
            {
                //printf ( "All flushed or not full packet!\n" );
                break;
            }

            TUPLE* tuple = new TUPLE ( result, outsize - 4, 0, 0 );
            
            // queue를 통해 처리되는 데이터
            if(tuple->gettype() == 0x02)
            {
                // 하드코딩;; 0번 익스큐터!
                STREAMFACTORY* factory = factorymanager.getStreamFactorybyid ( 0 );

                factory->installReceivedData ( tuple );
            }
            // db migration
            else if(tuple->gettype() == 0x04)
            {
                STREAMFACTORY* factory = factorymanager.getStreamFactorybyid ( 0 );
                STATEMANAGER* statemanager = factory->getStatemanager();
                statemanager->installDB( tuple );
            }
        }
        memset ( buffer, 0, sizeof buffer );
    }
}

void assemblePacket ( char* input, uint insize, deque< PACKET* >* packetque, unsigned char* seq )
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

char* getassembledPacket ( uint* outsize, deque< PACKET* >* packetque )
{
    vector<char> byte_array;
    deque<PACKET*> candidate;

    // 아무것도 없으면 나옴
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
    // 전체 패킷 양이 아직 풀패킷을 만들지 못할 정도일 때,
    unsigned int total_size = 0;
    for ( auto iter = packetque->begin(); iter != packetque->end(); ++iter )
    {
        PACKET* pkt = *iter;
        total_size += pkt->getSize();
    }

    if ( total_size < expectedsize )
    {
        return NULL;
    }


    candidate.clear();

    unsigned int accumulatedsize = 0;

    // 예상 패킷 사이즈만큼 모든 패킷 청크를 다 꺼내 candidate로 보냄
    while ( accumulatedsize < expectedsize )
    {
        PACKET* pkt = packetque->front();
        candidate.push_back ( pkt );
        accumulatedsize += pkt->getSize();
        packetque->pop_front();
        if ( packetque->size() == 0 )
            break;
        printf ( "accumulatedsize: %d expectedsize: %d\n", accumulatedsize, expectedsize );
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
