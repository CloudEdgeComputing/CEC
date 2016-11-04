#pragma once

#include "Type.h"
#include <list>

class CONNECTION;
class BASICCELL;
class PIPE;
class TUPLE;

using namespace std;

class STREAMFACTORY
{
private:
    PIPE* inpipe;
    list<PIPE*>* outpipelist;
    list<BASICCELL*> cells;
    CONNECTION* conn;
    bool state;
public:
    // STREAMFACTORY 생성자
    STREAMFACTORY();
    // STREAMFACTORY 파괴자
    ~STREAMFACTORY();
    
    // 익스큐터 내의 오퍼레이터들을 스케줄링 한다.
    void factoryStart();
    
    // STREAMFACTORY에 connection을 붙인다.
    void setCONNECTOR(CONNECTION* conn);
    // STREAMFACTORY가 가지고 있는 connection을 반환한다.
    CONNECTION* getCONNECTOR();
    // STREAMFACTORY의 inq를 세팅한다.
    void setinpipe(PIPE* inpipe);
    // STREAMFACTORY의 inq를 받는다.
    PIPE* getinpipe();
    // STREAMFACTORY의 outq를 세팅한다.
    void setoutpipe(PIPE* outpipe);
    // STREAMFACTORY outq를 받는다.
    list<PIPE*>* getoutpipelist();
    // STREAMFACTORY의 tasks 리스트를 받는다.
    list<BASICCELL*> getCELLs();
    // BASICCELL를 executor에 등록한다.
    void registerCELL(BASICCELL* cell);
    // STREAMFACTORY가 실행중인가?
    bool getSTREAMFACTORYState();
    // STREAMFACTORY 상태 변경 1, running 2, blocked
    bool setSTREAMFACTORYState(bool state);
    // Install received data
    void installReceivedData(TUPLE* data);
    // Debug
    void printtasks();
};