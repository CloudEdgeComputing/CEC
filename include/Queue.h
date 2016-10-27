#pragma once

#include "Type.h"
#include <pthread.h>
#include <list>
#include <vector>

#define TYPE_EXECUTOR   0
#define TYPE_TASK       1
#define TYPE_CONNECTION 2

using namespace std;

class Task;

// 큐는 operator에 속해 있다.
// 큐에는 그 큐를 깨우기 위해 wakeup을 할 수 있고, wakeup시, queue의 주인을 깨운다.
class QUEUE
{
private:
    lockfreeq* queue;
    // owner와 owner의 type관리
    list< pair<void*, int> > owner;
    // Migration이 완료된 Q인가?
    bool isMigrated;
public:
    QUEUE(lockfreeq* queue, void* owner, int type);
    lockfreeq* getQueue();
    // owner_type이 0이면 Executor, 1이면 Task, 2이면 Connection
    void registerDependency(void* owner, int type);
    bool sendQueue(int fd);
};