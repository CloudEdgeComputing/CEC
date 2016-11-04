#pragma once

#include "Type.h"
#include <pthread.h>
#include <list>
#include <vector>

#define TYPE_FACTORY    0
#define TYPE_CELL       1
#define TYPE_CONNECTION 2

#define UNDEFINED       0

using namespace std;

class BASICCELL;

// 큐는 operator에 속해 있다.
// 큐에는 그 큐를 깨우기 위해 wakeup을 할 수 있고, wakeup시, queue의 주인을 깨운다.
// owner_type이 0이면 Executor, 1이면 BASICCELL, 2이면 Connection
class PIPE
{
private:
    lockfreeq* queue;
    // owner와 owner의 type관리
    list< pair<void*, int> > back_dependency;
    list< pair<void*, int> > forward_dependency;
    // Migration이 완료된 Q인가?
    bool isMigrated;
    // queue의 전역 고유 아이디 // start from 0 to MAX(unsigned integer)
    unsigned int id;
public:
    
    PIPE(lockfreeq* queue, void* owner, int type, int id);
    lockfreeq* getQueue();
    // owner_type이 0이면 Executor, 1이면 BASICCELL, 2이면 Connection
    void registerbackDependency(void* dependency, int type);
    // owner_type이 0이면 Executor, 1이면 BASICCELL, 2이면 Connection
    void registerforwardDependency(void* dependency, int type);
    // fd에 해당하는 소켓에 queue 데이터를 몽땅 보낸다.
    bool sendQueue(int fd);
    // 큐의 글로벌 고유아이디를 가져온다.
    unsigned int getid();
    // Queue의 forward dependency에 해당하는 모든 것들을 깨운다.
    void install_comp_signal();
    // Migration이 완료된 플래그를 지운다.
    void clearMigration();
};
