#include "MIGRATION.h"
#include "STREAMFACTORY.h"
#include "PIPE.h"
#include "BASICCELL.h"
#include "FACTORYBUILDER.h"
#include "TUPLE.h"

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

void MIGRATION::startMIGRATION ( int id, STREAMFACTORY* factory )
{
    int sockfd = this->getClienctCEC ( id );

    // executor에 해당하는 모든 task들을 블럭시킨다.
    // Task scheduler task 블럭, 웨이크업

    auto list = factory->getCELLs();

    for ( auto iter = list.begin(); iter != list.end(); ++iter )
    {
        BASICCELL* cell = ( *iter );
        cell->schedulerSleep();
    }
    printf ( "all tasks in an executor goes to sleep\n" );

    // 블럭 되어있으니 이제 큐에 있는 데이터들을 전송한다.
    while ( 1 )
    {
        bool checker = false;
        PIPE* inpipe = factory->getinpipe();

        printf ( "from inq\n" );
        checker |= !inpipe->sendQueue ( sockfd );
        usleep ( 500 );

        // executor outq
        auto outpipelist = factory->getoutpipelist();
        printf ( "from outqs\n" );
        for ( auto iter = outpipelist->begin(); iter != outpipelist->end(); ++iter )
        {
            auto outpipe = *iter;
            checker |= !outpipe->sendQueue ( sockfd );
            usleep ( 500 );
        }

        auto list = factory->getCELLs();

        printf ( "from task\n" );
        for ( auto iter = list.begin(); iter != list.end(); ++iter )
        {
            BASICCELL* cell = *iter;
            // task에 있는 큐를 빼냄
            auto outpipelist = cell->getoutpipelist();
            for ( auto iter = outpipelist->begin(); iter != outpipelist->end(); ++iter )
            {
                auto outpipe = *iter;
                checker |= !outpipe->sendQueue ( sockfd );
                usleep ( 500 );
            }
        }

        if ( checker == false )
        {
            PIPE* inpipe = factory->getinpipe();
            
            inpipe->clearMigration();
            
            auto outpipelist = factory->getoutpipelist();

            for ( auto iter = outpipelist->begin(); iter != outpipelist->end(); ++iter )
            {
                auto outpipe = *iter;
                outpipe->clearMigration();
            }

            for ( auto iter = list.begin(); iter != list.end(); ++iter )
            {
                BASICCELL* cell = *iter;
                auto outpipelist = cell->getoutpipelist();
                for ( auto iter = outpipelist->begin(); iter != outpipelist->end(); ++iter )
                {
                    auto outpipe = *iter;
                    outpipe->clearMigration();
                }
                cell->schedulerWakeup();
            }
            printf ( "all cells in a factory goes to wake up\n" );
            break;
        }
    }

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
    char buffer[4096] = "";
    MIGRATION* migration = ( MIGRATION* ) context;

    // 여기는 0번째인걸 아니까.. 그냥 하드코딩..
    // 첫번째 등록된 엣지노드의 파일디스크립터
    // 제대로 하려면 select를 이용하여 입력다중화 시켜야 함
    // TODO
    int fd = migration->getServerCEC ( 0 );

    while ( 1 )
    {
        // 데이터를 받는다.
        unsigned int size = recv ( fd, buffer, sizeof buffer, 0 );
        printf ( "migration data is received size: %d\n", size );
        if ( ( size == -1 ) || ( size == 0 ) )
            break;

        // 받은 데이터를 큐에 설치한다.
        char* pdata = new char[size];
        memset ( pdata, 0, size );
        memcpy ( pdata, buffer, size );

        TUPLE* data = new TUPLE ( pdata, size - 4, 0, 0 );
        // 어떤 익스큐터인가?
        // 하드코딩;; 0번 익스큐터!
        STREAMFACTORY* factory = factorymanager.getStreamFactorybyid(0);

        // 설치한다.
        factory->installReceivedData( data );

        // 내부 변수 초기화
        delete data;
        delete []pdata;
        memset ( buffer, 0, sizeof buffer );
    }
}
