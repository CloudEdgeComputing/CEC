#pragma once

#include "Type.h"
#include <list>

class Connection;
class Task;
class Queue;
class DATA;

using namespace std;

class Executor
{
private:
    QUEUE* inq;
    list<QUEUE*>* outqlist;
    list<Task*> tasks;
    Connection* conn;
    bool state;
public:
    // Executor 생성자
    Executor();
    // Executor 파괴자
    ~Executor();
    
    // 익스큐터 내의 오퍼레이터들을 스케줄링 한다.
    void executorStart();
    
    // Executor에 connection을 붙인다.
    void setConnection(Connection* conn);
    // Executor가 가지고 있는 connection을 반환한다.
    Connection* getConnection();
    // Executor의 inq를 세팅한다.
    void setinq(QUEUE* inq);
    // Executor의 inq를 받는다.
    QUEUE* getinq();
    // Executor의 outq를 세팅한다.
    void setoutq(QUEUE* outq);
    // Executor outq를 받는다.
    list<QUEUE*>* getoutqlist();
    // Executor의 tasks 리스트를 받는다.
    list<Task*> getTask();
    // Task를 executor에 등록한다.
    void registerTask(Task* task);
    // Executor가 실행중인가?
    bool getExecutorState();
    // Executor 상태 변경 1, running 2, blocked
    bool setExecutorState(bool state);
    // Install received data
    void installReceivedData(DATA* data);
    // Debug
    void printtasks();
};