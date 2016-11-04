#pragma once

#include <stdlib.h>
#include <list>
#include <functional>
#include <pthread.h>
#include "Functions.h"
#include "CELL.h"

class PIPE;
class WORKER;
class STREAMFACTORY;
class CELL;

using namespace std;

class BASICCELL : public CELL 
{
private:
    // 어플리케이션 개발자가 지정할 수 있는 함수
    FUNC func;
    
public:
    // inq, outq를 갖는 taskpool을 만든다.
    BASICCELL( PIPE* inpipe, PIPE* outpipe, ushort count, FUNC func, STREAMFACTORY* parent );
    BASICCELL( PIPE* inpipe, list< PIPE* >* outpipelist, ushort count, FUNC func, STREAMFACTORY* parent );
    // Virtual, 워커를 만든다.
    virtual void makeWorker();
    // Virtual, 워커 스케줄링 내부 함수
    virtual void* scheduling(void* arg);
    // inpipe를 불러온다.
    PIPE* getinpipe();
    // func를 불러온다.
    FUNC getfunc();
};
