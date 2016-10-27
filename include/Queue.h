#pragma once

#include "Type.h"
#include <pthread.h>

class Task;

// 큐는 operator에 속해 있다.
// 큐에는 그 큐를 깨우기 위해 wakeup을 할 수 있고, wakeup시, queue의 주인을 깨운다.
class QUEUE
{
private:
    lockfreeq* queue;
    void* owner;
public:
    QUEUE(lockfreeq* queue, void* owner);
    lockfreeq* getQueue();
};