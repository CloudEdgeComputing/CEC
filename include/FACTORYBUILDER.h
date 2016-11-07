#pragma once

#include <list>
#include <sys/types.h>
#include <boost/lockfree/queue.hpp>
#include <termios.h>
#include <unistd.h>

class BASICCELL;
class STREAMFACTORY;
class TUPLE;
class PIPE;
class MIGRATION;

using namespace std;

class FACTORYBUILDER
{
private:
    list<STREAMFACTORY*> streamfactories;
    // unsigned int: id, QUEUE* QUEUE.. 자료구조 바꿔야함.. Global id가 확정적인 것으로.. 
    list<PIPE*> pipes;
    // QUEUE의 글로벌 아이디 관리
    int new_gid = 0;
    // 마이그레이션 모듈
    MIGRATION* migration;
public:
    // 익스큐터를 만든다. 필요 인자는 recvport
    STREAMFACTORY* createSTREAMFACTORY(ushort recvport);
    void runSTREAMFACTORY(STREAMFACTORY* factory);
    STREAMFACTORY* getStreamFactorybyid(int id);
    // 현재 lockfreequeue로 내부 구현, 바꿀 수 있음!
    PIPE* makePIPE(void* powner, int type, STREAMFACTORY* parent);
    PIPE* findPIPE(unsigned int id);
    
    // 마이그레이션 모듈과 연결
    void setMIGRATION(MIGRATION* mig);
    // 마이그레이션 모듈 받아오기
    MIGRATION* getMIGRATION();
    
    // 마이그레이션 스타트 디버깅
    void startMIGRATION();
};

extern FACTORYBUILDER factorymanager;