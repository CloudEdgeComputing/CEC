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
class TUPLE;

using namespace std;

class UNIONCELL : public CELL 
{
private:
    // 자체적으로 주어진 함수 -> 두개의 튜플을 합친다.
    MERGE_FUNC func;
    uint dominantpipeid;
    // PIPE 접근 시, 워커 한명만 접근 보장 되어야 하는 경우에 쓰는 락 (for UNIONCELL)
    pthread_mutex_t pipelock;
    
public:
    // inpipe, outpipe를 갖는 CELL을 만든다. deep copy
    UNIONCELL( list<PIPE*>* inpipelist, uint dominantpipeid, list< PIPE* >* outpipelist, ushort count, STREAMFACTORY* parent );
    // Virtual, 워커를 만든다.
    virtual void makeWorker();
    // Virtual, 워커 스케줄링 내부 함수
    virtual void* scheduling(void* arg);
    
    list< PIPE* >* getinpipelist();
    // MERGE_FUNC를 불러온다.
    MERGE_FUNC getfunc();
    // pipelock을 받는다.
    pthread_mutex_t* getpipelock();
    // dominantpipeid를 받는다.
    uint getdominantpipeid();
};

TUPLE* merge(list< TUPLE* >* tuplelist, uint dominantpipeid);