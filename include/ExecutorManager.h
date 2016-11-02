#pragma once

#include <list>
#include <sys/types.h>
#include <boost/lockfree/queue.hpp>
#include <termios.h>
#include <unistd.h>

class Task;
class Executor;
class DATA;
class QUEUE;
class Migration;

using namespace std;

class ExecutorManager
{
private:
    list<Executor*> execs;
    // unsigned int: id, QUEUE* QUEUE.. 자료구조 바꿔야함.. Global id가 확정적인 것으로.. 
    list<QUEUE*> ques;
    // QUEUE의 글로벌 아이디 관리
    int new_gid = 0;
    // 마이그레이션 모듈
    Migration* migration;
public:
    // 익스큐터를 만든다. 필요 인자는 recvport
    void createExecutor(ushort recvport);
    void executorRunAll();
    Executor* getExecutorbyId(int id);
    // 현재 lockfreequeue로 내부 구현, 바꿀 수 있음!
    QUEUE* makeQueue(void* powner, int type);
    QUEUE* findQueue(unsigned int id);
    
    // 마이그레이션 모듈과 연결
    void setMigrationModule(Migration* mig);
    // 마이그레이션 모듈 받아오기
    Migration* getMigrationModule();
    
    // 마이그레이션 스타트 디버깅
    void startMigration();
};

extern ExecutorManager executormanager;