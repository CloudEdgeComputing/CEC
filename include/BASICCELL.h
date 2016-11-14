#pragma once

/**
 *  @file BASICCELL.h
 *  @brief processing cell의 basic cell을 정의해놓은 헤더
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
class CELL;

using namespace std;


/**
 * @brief cell의 기본 형태를 상속하고 SISO의 형태를 갖고 있는 processing cell. 실제 processing의 유닛이다.
 * @details cell의 기본 형태를 상속하고 SISO의 형태를 갖고 있는 processing cell. 실제 processing의 유닛이다. SI는 한번에 하나의 튜플만을 받음을 의미하며 이 튜플들은 독립적으로 처리된다. SO는 하나의 튜플만이 나오며 outpipe에는 데이터가 각각 복사된다.
 * @author ysmoon
 * @date 2016-11-08
 * @version 0.0.1
 */
class BASICCELL : public CELL 
{
private:
    /**
     *  @brief cell에서 실제로 수행해야 할 코드 세그먼트. TUPLE* code(TUPLE* tuple) 함수 형태를 갖는다.
     */
    FUNC func;
    
public:
    /**
     *  @brief basiccell을 만드는 생성자
     *  @details basiccell을 만드는 생성자.
     *  @param inpipe cell에 대한 단일 input pipe이다.
     *  @param outpipe cell에 대한 단일 output pipe이다.
     *  @param count cell에 들어있는 worker의 개수를 정의한다.
     *  @param parent cell이 속한 factory를 명시한다.
     *  @param func 사용자가 정의할 수 있는 코드 세그먼트이다. TUPLE* code(TUPLE* tuple) 함수 형태를 갖는다.
     *  @param XIXO cell의 input과 output의 형태를 정의한다. SISO, MISO, SO, SI중 선택
     */
    BASICCELL( PIPE* inpipe, PIPE* outpipe, ushort count, FUNC func, STREAMFACTORY* parent );
    /**
     *  @brief basiccell을 만드는 생성자
     *  @details basiccell을 만드는 생성자.
     *  @param inpipe cell에 대한 단일 input pipe이다.
     *  @param outpipelist cell에 대한 output pipe list이다. 깊은 복사로 넘어간다. 이후 파라미터로 들어온것은 필요 없으면 메모리 해제 해야 한다.
     *  @param count cell에 들어있는 worker의 개수를 정의한다.
     *  @param parent cell이 속한 factory를 명시한다.
     *  @param func 사용자가 정의할 수 있는 코드 세그먼트이다. TUPLE* code(TUPLE* tuple) 함수 형태를 갖는다.
     *  @param XIXO cell의 input과 output의 형태를 정의한다. SISO, MISO, SO, SI중 선택
     */
    BASICCELL( PIPE* inpipe, list< PIPE* >* outpipelist, ushort count, FUNC func, STREAMFACTORY* parent );
    /**
     *  @brief 워커를 만든다. 
     *  @details 워커를 만든다. basiccell 생성자의 count에서 정의된 숫자만큼 functor를 수행하는 워커를 만든다. cell의 순수 가상 함수를 implementation한다.
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
     *  @brief 단일 inpipe를 반환하는 함수
     *  @details 단일 inpipe를 반환하는 함수. inpipe가 여러개이면 등록된 맨 첫번째 input pipe를 반환한다.
     *  @return PIPE*로써, inpipe의 포인터를 반환한다.
     */
    PIPE* getinpipe();
    /**
     *  @brief 코드 세그먼트를 반환하는 함수
     *  @details 코드 세그먼트를 반환하는 함수. TUPLE* code(TUPLE* tuple) 함수 형태를 갖는다.
     *  @return FUNC, FUNC는 typedef TUPLE* (*FUNC)(TUPLE*) 이다.
     */
    FUNC getfunc();
};
