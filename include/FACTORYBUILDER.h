#pragma once

#include <list>
#include <sys/types.h>
#include <boost/lockfree/queue.hpp>
#include <termios.h>
#include <unistd.h>

/**
 *  @file FACTORYBUILDER.h
 *  @brief factory를 만드는 툴인 factorybuilder를 정의해놓은 헤더
 */
class CELL;
class BASICCELL;
class UNIONCELL;
class SRCCELL;
class DESTCELL;
class STREAMFACTORY;
class TUPLE;
class PIPE;
class MIGRATION;

using namespace std;

/**
 * @brief factory builder를 만든다. 
 * @details factory builder를 통해 factory를 만들 수 있다. factory builder은 전체적인 factory 관리 또한 포함한다.
 * @author ysmoon
 * @date 2016-11-08
 * @version 0.0.1
 */

class FACTORYBUILDER
{
private:
    list<STREAMFACTORY*> streamfactories;
    // unsigned int: id, QUEUE* QUEUE.. 자료구조 바꿔야함.. Global id가 확정적인 것으로.. 
    list<PIPE*> pipes;
    // QUEUE의 글로벌 아이디 관리
    int new_gid = 0;
public:

    STREAMFACTORY* createSTREAMFACTORY(ushort recvport, MIGRATION* mig);
    void runSTREAMFACTORY(STREAMFACTORY* factory);
    STREAMFACTORY* getStreamFactorybyid(int id);
    // 현재 lockfreequeue로 내부 구현, 바꿀 수 있음!
    PIPE* makePIPE(void* powner, int type, STREAMFACTORY* parent);
    PIPE* findPIPE(unsigned int id);
};

extern FACTORYBUILDER factorymanager;