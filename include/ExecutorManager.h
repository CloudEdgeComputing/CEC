#pragma once

#include <list>
#include <sys/types.h>
#include <boost/lockfree/queue.hpp>
#include <termios.h>
#include <unistd.h>

class Task;
class Executor;
class DATA;

using namespace std;

class ExecutorManager
{
private:
    list<Executor*> execs;
public:
    // 익스큐터를 만든다. 필요 인자는 recvport
    void createExecutor(ushort recvport);
    void executorRunAll();
};