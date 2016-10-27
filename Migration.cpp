#include "Migration.h"
#include "Executor.h"
#include "Queue.h"
#include "Task.h"

Migration::Migration (int port)
{
    // Get Migration server list
    list<SERVER*> _list = this->getnearserver();
    
    // 접속을 기다릴 수 있는 서버를 만듦
    this->makeServer(port);
    
    // 인접한 서버 리스트에 대하여 커넥션 생성 (내가 서버인가 클라이언트인가를 지정)
    for(auto iter = _list.begin(); iter != _list.end(); ++iter)
    {
        SERVER* serv = *iter;
        int fd = this->makeConnection(serv->ip, serv->port, serv->isInstalled);
    }
    
    // 마이그레이션을 대기한다. (쓰레드 생성 필요)
    this->waitingMigration();
}

list< SERVER* > Migration::getnearserver()
{
    // 현재 하드코딩
    // 원래 설계는 전체 엣지 노드들을 관리하는 파라미터 서버같은것이 있고 거기에서 받아와야 함
    
    SERVER* server = new SERVER;
    server->ip = INADDR_ANY;
    server->port = 1238;
    server->isInstalled = false;
    this->nearserver.push_back(server);

    return this->nearserver;
}

int Migration::makeConnection ( char* ip, ushort port, bool isInstalled )
{
    if(isInstalled == true)
    {
        struct sockaddr_in serv_addr;
        int sockfd = socket(AF_INET, SOCK_STREAM, 0);
        
        serv_addr.sin_family = AF_INET;
        serv_addr.sin_port = port;
        inet_pton(AF_INET, ip, &(serv_addr.sin_addr));
        
        int result = connect( sockfd, (struct sockaddr*) &serv_addr, sizeof(serv_addr));
        if(result != 0)
        {
            printf("connection failed to CEC ip: %s\n", ip);
            exit(0);
        }
        this->registerCECs(sockfd);
    }
}

int Migration::makeServer ( ushort port )
{
    this->setport(port);
    
    pthread_t tid;
    pthread_create(&tid, NULL, internal_makeServer, (void*)this);
}

int internal_makeServer ( void* context )
{
    Migration* mig = (Migration*)context;
    
    int ret = 0;
    
    int sfd = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in addr;
    bzero(&addr, sizeof(addr));
    
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons (mig->getport());
    
    ret = ::bind(sfd, (struct sockaddr*)&addr, sizeof(addr));
    if( ret != 0)
    {
        printf("bind error in Migration server!\n");
        exit(0);
    }
    
    ret = ::listen(sfd, 5);
    if( ret != 0)
    {;    
    sockfd
        printf("listen error in Migration server!\n");
        exit(0);
    }
    printf("Migration server initialized!\n");
    
    while(1)
    {
        struct sockaddr_storage client_addr;
        int sin_size;
        int client_fd = accept(mig->getserver_fd(), (struct sockaddr *)&client_addr, &sin_size);
        mig->registerClient(client_fd);
    }
}

void Migration::startMigration ( int id, Executor* executor )
{
    int sockfd = this->getConnectingCEC(id);
    
    while(1)
    {
        bool checker = false;
        QUEUE* inq = executor->getinq();
        
        checker |= !inq->sendQueue(sockfd);
    
        // executor outq
        QUEUE* outq = executor->getoutq();
        checker |= !outq->sendQueue(sockfd);
        
        // Task를 검색하여 
        for(auto iter = executor->getTask().begin(); iter != executor->getTask().end(); ++iter)
        {
            Task* task = *iter;
            // task에 있는 큐를 빼냄
            QUEUE* queue = task->getoutq();
            checker |= !queue->sendQueue(sockfd);
        }
        
        if(checker == 0)
            break;
    }
    
    printf("Migration completed!\n");
}

void Migration::waitingMigration()
{
    // 쓰레드를 만들어 migration /TODO
}

int Migration::getConnectingCEC ( int n )
{
    int cnt = 1;
    for(auto iter = this->connecting_cecs.begin(); iter != this->connecting_cecs.end(); ++iter)
    {
        if(cnt == n)
            return *iter;
        else
            cnt++;
    }
}

void Migration::registerClient ( int fd )
{
    this->connected_clients.push_back(fd);
}

void Migration::registerCECs ( int fd )
{
    this->connecting_cecs.push_back(fd);
}

bool Migration::getisInstalled()
{
    return this->isInstalled;
}

void Migration::setisInstalled ( bool sw )
{
    this->isInstalled = sw;
}