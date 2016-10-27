#pragma once

#include <sys/socket.h>
#include <netinet/in.h>
#include <strings.h>
#include <iostream>
#include <list>
#include <pthread.h>
#include <arpa/inet.h>

using namespace std;

class Executor;
class QUEUE;

struct SERVER
{
    char ip[20];
    ushort port;
    bool isInstalled;
} Server;

class Migration
{
private:
    bool isInstalled;
    int server_fd;
    ushort server_port;
    sockaddr_in addr;
    list<Server*> nearserver;
    list<int> connected_clients;
    list<int> connecting_cecs;
public:
    // 마이그레이션 모듈을 설치한다.
    // 이 모듈은 현재 CEC Framework과 다른 CEC framework간의 커넥션을 생성한다.
    Migration(char* ip, ushort port);
    // 인접 CEC의 서버 정보들을 받아온다.
    list<Server*> getnearserver();
    // 커넥션을 받을 수 있는 서버 생성
    int makeServer(ushort port);
    // 커넥션 생성
    int makeConnection(char* ip, ushort port, bool isClient);
    // 모든 큐 데이터들을 긁어 마이그레이션을 시작한다. input: target node id
    void startMigration(int id);
    // 마이그레이션을 대기한다.
    void waitingMigration();
    // Queue의 데이터를 Serialization 및 지정된 소켓 디스크립터로 보낸다.
    void sendQueue(QUEUE* Q, int fd);
    
    // methods
    // 생성된 서버 디스크립터 세팅
    void setserver_fd(int fd);
    // 서버 디스크립터 리턴
    int getserver_fd();
    // 포트 세팅
    void setport(ushort port);
    // 클라이언트 등록
    void registerClient(int fd);
    // 타겟 CEC 등록
    void registerCECs(int fd);
    // 포트 가져오기
    ushort getport();
    // 마이그레이션 모듈이 설치 되어있는가?
    bool getisInstalled();
    // 마이그레이션 모듈을 설치했다고 설정
    void setisInstalled(bool sw); 
    // 연결하고 있는 CEC중 n번째의 소켓 디스크립터를 가져옴
    int getConnectingCEC(int n);
};

static int internal_makeServer ( void* context );