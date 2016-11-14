#pragma once

/**
 *  @file UNIONCELL.h
 *  @brief 두개 이상의 tuple을 합쳐주는 cell
 */


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

#define POLICY_ALLREADY         0
#define POLICY_PARTIALREADY     1

using namespace std;
/**
 * @brief cell의 기본 형태를 상속하고 MISO의 형태를 갖고 있는 union cell이다. (processing cell이 아니다.) MISO의 형태를 갖는다.
 * @details cell의 기본 형태를 상속하고 MISO의 형태를 갖고 있는 union cell이다. (processing cell이 아니다.) MISO의 형태를 갖는다. 단순히 두개 이상의 튜플을 합쳐준다. 예를들어 하나의 튜플은 [int, char] 데이터를 갖고 있고 다른 하나의 튜플은 [char, char]를 갖고 있으면 합쳐지면 [int, char, char, char]의 튜플을 갖게 된다.
 * @author ysmoon
 * @date 2016-11-08
 * @version 0.0.1
 */
class UNIONCELL : public CELL 
{
private:
    // 자체적으로 주어진 함수 -> 두개의 튜플을 합친다.
    MERGE_FUNC func;
    uint dominantpipeid;
    // PIPE 접근 시, 워커 한명만 접근 보장 되어야 하는 경우에 쓰는 락 (for UNIONCELL)
    pthread_mutex_t pipelock;
    // union시, policy 설정
    uint union_policy;
    
public:
    /**
     *  @brief unioncell을 만드는 생성자. 코드세그먼트는 framework 내부적으로 정의되어 있는 것을 이용한다.
     *  @details unioncell을 만드는 생성자. 코드세그먼트는 framework 내부적으로 정의되어 있는 것을 이용한다.
     *  @param inpipelist cell에 대한 input pipe list이다. 깊은 복사로 넘어간다. 이후 파라미터로 들어온것은 필요 없으면 메모리 해제 해야 한다.
     *  @param dominantpipeid 두개 이상의 튜플을 합칠 때, 결과 튜플의 속성에 어떤 파이프에 있던 튜플 속성을 합칠 지 정의한다. 예를들어 1번파이프와 2번파이프가 연결되어 있고 dominantpipeid가 1번 파이프로 지정되면, 결과튜플은 항상 1번 파이프의 속성을 갖는다.
     *  @param outpipe cell에 대한 단일 output pipe이다.
     *  @param count cell에 들어있는 worker의 개수를 정의한다. worker는 단순히 두개 이상의 튜플을 합치는 일을 한다.
     *  @param parent cell이 속한 factory를 명시한다.
     */
    UNIONCELL( list<PIPE*>* inpipelist, uint dominantpipeid, list< PIPE* >* outpipelist, ushort count, STREAMFACTORY* parent, uint union_policy );
    /**
     *  @brief 워커를 만든다. 
     *  @details 워커를 만든다. unioncell 생성자의 count에서 정의된 숫자만큼 합치는 기능을 수행하는 워커를 만든다. cell의 순수 가상 함수를 implementation한다.
     *  @return void 의미 없음
     */
    virtual void makeWorker();
    /**
     *  @brief 만든 워커를 실제로 스케줄링 하는 함수
     *  @details 워커를 실제로 스케줄링 하는 함수이다. cell의 scheduling_wrapper에 의해 불린다. while문을 돌면서 iput pipe들을 풀링한다.
     *  @return void 의미 없음
     *  @bug 스케줄링에서 outpipe로 데이터를 넘길 때, pipe에 각각 데이터를 깊은복사를 해서 넘겨주어야 한다.
     */
    virtual void* scheduling(void* arg);
    /**
     *  @brief cell에 연결된 inpipe list를 반환하는 함수
     *  @details cell에 연결된 inpipe list를 반환하는 함수
     *  @return list<PIPE*>*로써, 파이프 리스트 포인터를 반환받는다.
     */
    list< PIPE* >* getinpipelist();
    /**
     *  @brief 코드 세그먼트를 반환하는 함수
     *  @details 코드 세그먼트를 반환하는 함수. 
     *  @return MERGE_FUNC로, TUPLE* merge(list< TUPLE* >* tuplelist, uint dominantpipeid) 함수 형태를 갖는 포인터를 반환한다.
     */
    MERGE_FUNC getfunc();
    /**
     *  @brief input pipe들의 락을 반환하는 함수
     *  @details input pipe들의 락을 반환하는 함수. unioncell은 워커가 두개 이상의 파이프에 접근을 하기 때문에, 관리 리스트등이 atomic operation을 제대로 수행하지 못해 오류가 날 수 있다. 그러므로 락을 걸어 보호한다. 
     *  @return pthread_mutex_t* 로써, 락의 포인터를 반환한다.
     *  @todo pipe락을 없애는 방법이 있으면 없애기
     */
    pthread_mutex_t* getpipelock();
    /**
     *  @brief dominantpipeid를 반환받는다.
     *  @details dominantpipeid를 반환받는다. dominantpipeid는 두개 이상의 튜플을 합칠 때, 결과 튜플의 속성에 어떤 파이프에 있던 튜플 속성을 합칠 지 정의한다. 예를들어 1번파이프와 2번파이프가 연결되어 있고 dominantpipeid가 1번 파이프로 지정되면, 결과튜플은 항상 1번 파이프의 속성을 갖는다.
     *  @return dominantpipeid
     */
    uint getdominantpipeid();

    /**
     *  @brief union policy를 받는다.
     *  @details union policy를 받는다. POLICY_ALLREADY, POLICY_PARTIALREADY를 반환
     *  @return POLICY_ALLREADY(0), POLICY_PARTIALREADY(1)를 반환
     */
    uint getUnionPolicy();
};

/**
 *  @brief 두개 이상의 튜플을 merge를 수행하는 내부 로직
 *  @details 두개 이상의 튜플을 merge하는 내부 로직이다. 다수의 튜플은 리스트 형태로 들어가며, 또한 결과 튜플의 속성을 정의하기 위한 dominantpipeid가 필요하다.
 *  @return TUPLE*로써, 하나의 튜플 포인터를 반환한다.
 */
TUPLE* merge(list< TUPLE* >* tuplelist, uint dominantpipeid);