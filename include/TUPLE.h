#pragma once

/**
 *  @file TUPLE.h
 *  @brief data의 교환단위인 tuple을 정의해놓은 헤더
 */

#include "Type.h"
#include <string.h>
#include <netinet/in.h>

typedef unsigned short ushort;
typedef unsigned char uchar;
/**
 * @brief data의 교환단위인 tuple을 정의한 클래스이다.
 * @details data의 교환단위인 tuple을 정의한 클래스이다. 실제 내부는 byte array로 되어 있으며, 튜플의 프로토콜에 따라 byte array가 배열되어 있다. 프로토콜은 다음과 같다.
 * [0xAA]: 튜플의 프로토콜을 정의하거나 integrity를 체크할 수 있는 바이트이다.
 * [size(2byte)]: 튜플의 프로토콜을 제외한 데이터 사이즈를 나타낸다. 최대 255byte를 가질 수 있다.
 * [type(1byte)]: 튜플 데이터가 가지고 있는 data type을 나타낸다. 또한 attribute로도 reserved 되어 있다.
 * @author ysmoon
 * @date 2016-11-08
 * @version 0.0.1
 */
class TUPLE
{
private:
    char* data;
    char* content;
    ushort nLen;
    ushort seq;
    int uuid;
    uint pointer;
    uint pipeid;

public:
    /**
     *  @brief TUPLE을 만드는 생성자. 이 생성자는 프로토콜을 가진 raw data에서 tuple로 전환하기 위한 생성자이다.
     *  @details TUPLE을 만드는 생성자. 이 생성자는 프로토콜을 가진 raw data에서 tuple로 전환하기 위한 생성자이다. 보통 데이터를 받았을 때, 튜플로 전환하기 위해 쓰인다.
     *  @param data 튜플에 들어갈 byte array. 이 byte array는 튜플의 프로토콜 또한 포함하고 있어야 한다.
     *  @param nLen 프로토콜을 제외한 byte array의 사이즈
     *  @param seq 튜플의 시퀀스를 나타낸다. reserved 되어 있다.
     *  @param uuid 원산지 디바이스 uuid
     */
    TUPLE(char* data, ushort nLen, ushort seq, int uuid);
    /**
     *  @brief TUPLE을 만드는 생성자. 이 생성자는 새로운 튜플을 만들어야 할 때 쓰인다.
     *  @details TUPLE을 만드는 생성자. 이 생성자는 새로운 튜플을 만들어야 할 때 쓰인다. 즉, 어떤 생성된 데이터를 튜플화 시키고자 할 때 쓰인다.
     *  @param nLen 튜플이 가지고 있을 byte array의 사이즈를 확보한다.
     *  @param uuid 원산지 디바이스 uuid
     *  @param type 튜플의 type(attribute)를 설정한다.
     */
    TUPLE(ushort nLen, int uuid, int type);
    /**
     *  @brief TUPLE을 만드는 생성자. 이 생성자는 migration에 쓰인다. (migration은 특수 프로토콜을 쓰기 때문..)
     *  @details TUPLE을 만드는 생성자. 이 생성자는 migration에 쓰인다. (migration은 특수 프로토콜을 쓰기 때문..)
     *  [0xAA][size(2byte)][type(1byte)] 까지는 일반적인 프로토콜을 따른다. 단 내용에 [4byte for ipv4]와 [4byte for pipe id]가 추가된다. 그 뒤, 실제적 내용이 추가된다.
     *  @param packet 특수 프로토콜을 가진 byte array가 input으로 들어간다.
     *  @param isSpecial 패킷이 특수 프로토콜을 가지는지 설정한다. TRUE면 특수 프로토콜을 사용한다고 가정한다.
     *  @todo isSpecial이 false인경우 프로그램이 종료된다. (미구현)
     */
    TUPLE ( char* packet, bool isSpecial );
    
    /**
     *  @brief 튜플이 유효한 튜플인지 체크한다.
     *  @details 튜플이 유효한 튜플인지 체크한다. 내부에서는 0xAA 바이트를 체크한다.
     *  @return true인 경우 유효한 패킷이고, false인 경우, 유효하지 않은 패킷이다.
     */
    bool validity();
    /**
     *  @brief 튜플의 type 또는 attribute를 가져온다.
     *  @details 튜플의 type 또는 attribute를 가져온다.
     *  0x00: 일반적인 튜플
     *  0x02: 마이그레이션을 위한 튜플
     *  나머지는 reserved되어 있다.
     *  @return 튜플의 type을 리턴한다.
     */
    uchar gettype();
    /**
     *  @brief 튜플이 가지고 있는 프로토콜을 제외한 byte array의 포인터를 받아온다.
     *  @details 튜플이 가지고 있는 프로토콜을 제외한 byte array의 포인터를 받아온다.
     *  @return 프로토콜을 제외한 byte array의 포인터
     */
    char* getcontent();
    /**
     *  @brief 튜플이 가지고 있는 프로토콜을 포함한 byte array의 포인터를 받아온다.
     *  @details 튜플이 가지고 있는 프로토콜을 포함한 byte array의 포인터를 받아온다.
     *  @return 프로토콜을 포함한 byte array의 포인터
     */
    char* getdata();
    /**
     *  @brief 튜플이 가지고 있는 프로토콜을 제외한 byte array의 사이즈를 받아온다.
     *  @details 튜플이 가지고 있는 프로토콜을 제외한 byte array의 사이즈를 받아온다.
     *  @return 튜플이 가지고 있는 프로토콜을 제외한 byte array의 사이즈
     */
    ushort getLen();
    /**
     *  @brief 이 튜플의 원산지 디바이스 uuid를 받아온다.
     *  @details 이 튜플의 원산지 디바이스 uuid를 받아온다.
     *  @return 받아온 unsigned integer uuid
     */
    unsigned int getuuid();
    /**
     *  @brief 현재 이 튜플의 원산지 디바이스 uuid를 바꾸거나 세팅한다.
     *  @details 
     *  @param uuid 새로 설정한 uuid
     */
    void setuuid(unsigned int uuid);
    /**
     *  @brief TUPLE이 현재 존재하거나 존재 했던 파이프의 최신 아이디를 세팅한다. 
     *  @details TUPLE이 현재 존재하거나 존재 했던 파이프의 최신 아이디를 세팅한다. 이 아이디는 dominant id를 비교하기 위해 쓰인다.
     *  @param pipeid tuple에 넘겨 줄 파이프의 최신 아이디
     */
    void setpipeid(uint pipeid);
    /**
     *  @brief TUPLE이 현재 존재하거나 존재 했던 파이프의 최신 아이디를 받아온다.
     *  @details TUPLE이 현재 존재하거나 존재 했던 파이프의 최신 아이디를 받아온다. 이 아이디는 dominant id를 비교하기 위해 쓰인다.
     *  @return TUPLE이 현재 존재하거나 존재 했던 파이프의 최신 아이디
     */
    uint getpipeid();
    
    
    
    /**
     *  @brief 실제 프로토콜을 제외한 데이터에서 앞에서 4byte를 빼서 integer로 받아온다.
     *  @details 실제 프로토콜을 제외한 데이터에서 앞에서 4byte를 빼서 integer로 받아온다. 한번 빼온 integer는 이 메소드를 통해 받아올 수 없다.
     *  @return 4byte integer
     */
    int getInt();
    /**
     *  @brief 실제 프로토콜을 제외한 데이터에서 앞에서 4byte를 빼서 float로 받아온다.
     *  @details 실제 프로토콜을 제외한 데이터에서 앞에서 4byte를 빼서 float로 받아온다. 한번 빼온 float는 이 메소드를 통해 받아올 수 없다.
     *  @return 4byte float
     */
    float getFloat();
    /**
     *  @brief 실제 프로토콜을 제외한 데이터에서 앞에서 정해진 사이즈만큼 빼서 byte array로 받아온다.
     *  @details 실제 프로토콜을 제외한 데이터에서 앞에서 정해진 사이즈만큼 빼서 byte array로 받아온다. 한번 빼온 byte array는 이 메소드를 통해 받아올 수 없다.
     *  @param str 실제 byte array가 저장될 배열. 이 배열은 미리 충분한 크기로 초기화가 되어 있어야 한다.
     *  @param nSize 받아올 byte array의 사이즈
     */
    void getString(char* str, int nSize);
    /**
     *  @brief 실제 프로토콜을 제외한 데이터에서 앞에서 2byte를 빼서 short로 받아온다.
     *  @details 실제 프로토콜을 제외한 데이터에서 앞에서 2byte를 빼서 short로 받아온다. 한번 빼온 short는 이 메소드를 통해 받아올 수 없다.
     *  @return short 값
     */
    short getShort();
    /**
     *  @brief 실제 프로토콜을 제외한 데이터에서 앞에서 1byte를 빼서 char로 받아온다.
     *  @details 실제 프로토콜을 제외한 데이터에서 앞에서 1byte를 빼서 char로 받아온다. 한번 빼온 char는 이 메소드를 통해 받아올 수 없다.
     *  @return char 값
     */
    char getChar();
    
    
    
    
    // Data maker method
    /**
     *  @brief Tuple 내부에 데이터를 넣는다. 큐 형태로 계속 푸쉬 된다.
     *  @details Tuple 내부에 데이터를 넣는다. 큐 형태로 계속 푸쉬 된다.
     *  @param data 넣을 데이터의 포인터
     *  @param size 데이터의 사이즈
     */
    void push(void* data, ushort size);
    /**
     *  @brief 이제까지 넣은 데이터 사이즈를 계산해 튜플 프로토콜을 완성시킨다. (사이즈를 재구성 한다.)
     *  @details 이제까지 넣은 데이터 사이즈를 계산해 튜플 프로토콜을 완성시킨다. (사이즈를 재구성 한다.)
     */
    void sealing();
};