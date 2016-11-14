#pragma once

#include "Type.h"
#include <list>

class CELL;
class PIPE;
class TUPLE;
class MIGRATION;
class STATEMANAGER;

using namespace std;

class STREAMFACTORY
{
private:
    list<CELL*> cells;
    bool state;
    list<CLIENT*> devices;
    MIGRATION* mig;
    STATEMANAGER* statemanager;
public:
    // STREAMFACTORY 생성자
    STREAMFACTORY(MIGRATION *mig);
    // STREAMFACTORY 파괴자
    ~STREAMFACTORY();
    
    // 익스큐터 내의 오퍼레이터들을 스케줄링 한다.
    void factoryStart();
    // STREAMFACTORY의 tasks 리스트를 받는다.
    list<CELL*> getCELLs();
    // BASICCELL를 executor에 등록한다.
    void registerCELL(CELL* cell);
    // STREAMFACTORY가 실행중인가?
    bool getSTREAMFACTORYState();
    // STREAMFACTORY 상태 변경 1, running 2, blocked
    bool setSTREAMFACTORYState(bool state);
    // Install received data
    void installReceivedData(TUPLE* data);
    // 현재 접속한 디바이스를 얻는다.
    list<CLIENT*>* getsrcdevices();
    // ip가 일치하는 CLIENT를 얻는다.
    CLIENT* getClientbyip(unsigned int ip);
    // fd가 일치하는 client를 얻는다.
    CLIENT* getClientbyuuid(unsigned int uuid);
    // 스트림 팩토리의 uuid에 해당하는 모든 데이터를 마이그레이션을 실행한다.
    void startMIGRATION(unsigned int uuid);
    // 스트림 팩토리에 달려있는 statemanager를 호출한다.
    STATEMANAGER* getStatemanager();
    // Debug
    void printtasks();
};