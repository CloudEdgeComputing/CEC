#pragma once

#include <vector>
#include <iostream>
#include <stdlib.h>  
#include <unistd.h>  
#include <errno.h>  
#include <string.h>  
#include <sys/types.h>  
#include <sys/socket.h>  
#include <netinet/in.h>  
#include <netdb.h>  
#include <arpa/inet.h>  
#include <sys/wait.h>  
#include <pthread.h>  
#include <signal.h>  
#include <list>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <pthread.h>
#include "Type.h"

using namespace std;

class QUEUE;

class Connection
{
private:
    pthread_t dispatcher_tid, sender_tid;
    list<pthread_t> receiver_tids;
public:
    int _broker_sock;
    sockaddr_in _broker_addr;
    list<struct CLIENT*> clientlists;
    
    // Executor에 대한 connection을 만든다. 인자는 recvport로 쓸 숫자를 받는다.
    Connection(int recvport);
    // Connection 해제
    ~Connection();
    // serverStart wrapper
    static void* serverStart_wrapper(void* context);
    // serverStart internal
    void* serverStart_internal(void* arg);
    // sender, receivers, dispatcher 생성
    pthread_t serverStart(QUEUE* inq, QUEUE* outq);
    // ip주소를 뽑는 함수
    void* get_in_addr( struct sockaddr *sa );
    // 패킷을 보내는 함수
    void* sender(void* arg);
    // sender wrapper
    static void* sender_wrapper( void* context );
    // 패킷을 받는 함수
    void* receiver(void* arg);
    // receiver wrapper
    static void* receiver_wrapper(void* context);
    // 리비서에서 받은 패킷을 분류하는 함수
    void* dispatcher(void* arg);
    // dispatcher wrapper
    static void* dispatcher_wrapper ( void* context );
    
    void setsender_tid ( pthread_t tid );
    void setdispatcher_tid ( pthread_t tid );
    //void register_user(pthread_t tid, int fd, struct sockaddr_storage client_addr);
    
};