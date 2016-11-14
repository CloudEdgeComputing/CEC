#pragma once

/**
 *  @file CELL.h
 *  @brief processing cell의 기본단위인 cell의 기본 형태를 정의해놓은 헤더
 */

#include <stdlib.h>
#include <list>
#include <functional>
#include <pthread.h>
#include "Functions.h"

class PIPE;
class WORKER;
class STREAMFACTORY;

using namespace std;

#define SISO    0
#define MISO    1
#define SO      2
#define SI      3

/**
 * @brief processing cell의 기본 구현이 들어있는 클래스
 * @details processing cell의 기본 구현이 들어있는 클래스이다. 이 클래스는 파생되는 셀에 상속된다.
 * @author ysmoon
 * @date 2016-11-08
 * @version 0.0.1
 */
class CELL
{
    
protected:
    list<PIPE*>* inpipelist;
    list<PIPE*>* outpipelist;
    ushort count;
    STREAMFACTORY* parent;
    bool isrun;
    bool isEnd;
    pthread_t scheduling_tid;
    list<WORKER*> workers;
    ushort XIXO;
    
    // 스케줄링 쓰레드 시그널 컨디션 변수
    pthread_cond_t condition;
    // 스케줄링 쓰레드 시그널 컨디션 변수의 락
    pthread_mutex_t cond_mutex;
    // 스케줄링 isRun 변수 락
    pthread_mutex_t mutex;
    
public:
    /**
     *  @brief cell을 만드는 생성자
     *  @details cell을 만드는 생성자. 보통은 상속할 때 상속 생성자로 불리는 생성자이다.
     *  @param inpipe cell에 대한 단일 input pipe이다.
     *  @param outpipe cell에 대한 단일 output pipe이다.
     *  @param count cell에 들어있는 worker의 개수를 정의한다.
     *  @param parent cell이 속한 factory를 명시한다.
     *  @param XIXO cell의 input과 output의 형태를 정의한다. SISO, MISO, SO, SI중 선택
     */
    CELL( PIPE* inpipe, PIPE* outpipe, ushort count, STREAMFACTORY* parent, uint XIXO );
    /**
     *  @brief cell을 만드는 생성자. 일반적으로 XIXO가 SO, SI일때 쓰인다. 
     *  @details cell을 만드는 생성자. 일반적으로 XIXO가 SO, SI일때 쓰인다. 보통은 상속할 때 상속 생성자로 불리는 생성자이다.
     *  @param inoutpipe input 또는 output pipe로 쓰일 단일 파이프를 지정한다.
     *  @param count cell에 들어있는 worker의 개수를 정의한다.
     *  @param parent cell이 속한 factory를 명시한다.
     *  @param XIXO cell의 input과 output의 형태를 정의한다. SISO, MISO, SO, SI중 선택
     */
    CELL( PIPE* inoutpipe, ushort count, STREAMFACTORY* parent, uint XIXO );
    /**
     *  @brief cell을 만드는 생성자.
     *  @details cell을 만드는 생성자. 보통은 상속할 때 상속 생성자로 불리는 생성자이다.
     *  @param inpipe cell에 대한 단일 input pipe이다.
     *  @param outpipelist cell에 대한 output pipe list이다. 깊은 복사로 넘어간다. 이후 파라미터로 들어온것은 필요 없으면 메모리 해제 해야 한다.
     *  @param count cell에 들어있는 worker의 개수를 정의한다.
     *  @param parent cell이 속한 factory를 명시한다.
     *  @param XIXO cell의 input과 output의 형태를 정의한다. SISO, MISO, SO, SI중 선택
     */
    CELL( PIPE* inpipe, list<PIPE*>* outpipelist, ushort count, STREAMFACTORY* parent, uint XIXO );
    /**
     *  @brief cell을 만드는 생성자. 일반적으로 SO일때 쓰인다.
     *  @details cell을 만드는 생성자. 일반적으로 SO일때 쓰인다. 보통은 상속할 때 상속 생성자로 불리는 생성자이다.
     *  @param outpipelist cell에 대한 output pipe list이다. 깊은 복사로 넘어간다. 이후 파라미터로 들어온것은 필요 없으면 메모리 해제 해야 한다.
     *  @param count cell에 들어있는 worker의 개수를 정의한다.
     *  @param parent cell이 속한 factory를 명시한다.
     *  @param XIXO cell의 input과 output의 형태를 정의한다. SISO, MISO, SO, SI중 선택
     */
    CELL( list<PIPE*>* outpipelist, ushort count, STREAMFACTORY* parent, uint XIXO );
    /**
     *  @brief cell을 만드는 생성자.
     *  @details cell을 만드는 생성자. 보통은 상속할 때 상속 생성자로 불리는 생성자이다.
     *  @param inpipe cell에 대한 단일 input pipe list이다. 깊은 복사로 넘어간다. 이후 파라미터로 들어온것은 필요 없으면 메모리 해제 해야 한다.
     *  @param outpipelist cell에 대한 output pipe list이다. 깊은 복사로 넘어간다. 이후 파라미터로 들어온것은 필요 없으면 메모리 해제 해야 한다.
     *  @param count cell에 들어있는 worker의 개수를 정의한다.
     *  @param parent cell이 속한 factory를 명시한다.
     *  @param XIXO cell의 input과 output의 형태를 정의한다. SISO, MISO, SO, SI중 선택
     */
    CELL( list<PIPE*>* inpipelist, list<PIPE*>* outpipelist, ushort count, STREAMFACTORY* parent, uint XIXO );
    
    /**
     *  @brief cell의 워커를 만드는 순수 가상 메소드
     *  @details 실제 워커를 쓰레드화 시킨다. 사용자 지정 함수도 여기에서 할당된다. 순수 가상 메소드이므로 상속하면 구현이 필수적이다.
     *  @return void
     */
    virtual void makeWorker() = 0;
    
    /**
     *  @brief cell안의 워커를 스케줄링 및 실행한다.
     *  @details cell안의 워커를 스케줄링 및 실행한다. 내부에서 쓰레드를 만들어 최종적으로 scheduling 함수를 호출한다. schedulingStart -> scheduling_wrapper -> scheduling 순으로 작동한다.
     *  @return void
     */
    void schedulingStart();
    
    /**
     *  @brief 실제 스케줄러의 내부 알고리즘이다. 순수 가상 메소드이다.
     *  @details 실제 스케줄러의 내부 알고리즘이다. 순수 가상 메소드이다. 즉, 파생 셀마다 스케줄링 내부 알고리즘이 달라질 수 있으므로 상속 대상자에서 함수를 정의해야 한다.
     *  @param arg cell의 this 포인터이다. 즉, 상속클래스로 포인터 캐스팅하여 이용할 수 있다. 하지만 현재까지 이용하는 경우 없음
     *  @return void* 실제로는 보통 NULL을 리턴하고 의미는 없다.
     */
    virtual void* scheduling(void* arg) = 0;
    
    /**
     *  @brief 내부 스케줄링 알고리즘인 scheduling을 호출한다. 
     *  @details 내부 스케줄링 알고리즘인 scheduling을 호출한다. 이 래퍼의 존재 의의는 this pointer를 넘겨주기 위함이다.
     *  @param context cell의 this 포인터이다.
     *  @return void* 실제로는 보통 NULL을 리턴하고 의미는 없다.
     */
    static void* scheduling_wrapper(void* context);
    
    /*
     *  @brief 워커가 하나 이상이 수행중이면 true를 반환한다. 모든 워커가 블록 되어있으면 false를 리턴한다.
     *  @details 워커가 하나 이상이 수행중이면 true를 반환한다. 모든 워커가 블록 되어있으면 false를 리턴한다.
     *  @return bool
     */
    //bool getWORKERState();
    
    /**
     *  @brief cell에 연결된 outpipe의 list를 반환한다.
     *  @details cell에 연결된 outpipe의 list를 반환한다.
     *  @return list<PIPE*>* 리스트 포인터를 반환한다.
     */
    list<PIPE*>* getoutpipelist();
    
    /**
     *  @brief cell의 스케줄러를 블럭한다.
     *  @details cell의 스케줄러를 블럭한다. 동작 즉시 워커가 블럭되지는 않는다. 즉, 스케줄러 블럭 후 워커가 일을 마치면 차례대로 블럭된다. cell의 state를 false로 변환시킨다.
     *  @return void
     */
    void schedulerSleep();
    
    /**
     *  @brief cell의 스케줄러를 깨운다.
     *  @details cell의 스케줄러를 깨운다. 스케줄러를 깨우면 다시 스케줄러라 input pipe를 풀링한다.
     *  @return void
     */
    void schedulerWakeup();
    
    /**
     *  @brief cell의 스케줄러 상태를 반환한다.
     *  @details cell의 스케줄러 상태를 반환한다. cell의 상태는 워커 상태와 다르며 cell의 스케줄러 상태가 러닝 상태이면 true, 블럭 상태면 false를 반환한다.
     *  @return bool
     */
    bool getCELLState();
    
    /**
     *  @brief cell의 입출력 형태를 반환한다.
     *  @details cell의 입출력 형태를 반환한다. SISO, MISO, SI 그리고 SO중 하나를 리턴한다.
     *  @return ushort SISO(0) MISO(1) SO(2) SI(3)을 리턴한다.
     */
    ushort getXIXO();
    
    /**
     *  @brief cell의 parent를 반환한다.
     *  @details 
     *  @return cell의 parent streamfactory object poitner
     */
    STREAMFACTORY* getParentFactory();
    /**
     *  @brief cell에서 설정된 uuid는 처리하지 않도록 한다.
     *  @details cell에서 설정된 uuid는 처리하지 않도록 한다.
     */
    void setignoreuuid(unsigned uuid);
    
    /**
     *  @brief ignore된 uuid를 모두 clear 한다.
     *  @details ignore된 uuid를 모두 clear 한다.
     */
    void clearignoreuuid();
    
    /**
     *  @brief cell이 특정 uuid의 tuple을 처리하고 있는지 본다.
     *  @details 처리중이면 true, 처리중이지 않으면 false를 반환한다.
     *  @return bool
     */
    bool hasuuid(unsigned int uuid);
};
