#include "PIPE.h"
#include "BASICCELL.h"
#include "WORKER.h"
#include "STREAMFACTORY.h"
#include "TUPLE.h"
#include "STREAMFACTORY.h"
#include "Debug.h"

PIPE::PIPE ( lockfreeq* queue, void* owner, int type, int id, STREAMFACTORY* parent )
{
    this->queue = queue;
    if ( owner != NULL )
    {
        this->back_dependency.push_back ( make_pair ( owner, type ) );
    }
    this->isMigrated = false;
    this->id = id;
    this->parent = parent;
}

lockfreeq* PIPE::getQueue()
{
    return this->queue;
}

void PIPE::registerbackDependency ( void* owner, int type )
{
    this->back_dependency.push_back ( make_pair ( owner, type ) );
}

void PIPE::registerforwardDependency ( void* owner, int type )
{
    this->forward_dependency.push_back ( make_pair ( owner, type ) );
}

bool PIPE::sendQueue ( int fd )
{
    if ( this->isMigrated == true )
    {
        return true;
    }

    // Queue의 Dependency 체크
    // 모든 Dependency를 가진 것들이 멈춰야 한다.
    // 안멈춰 있으면 False return
    for ( auto iter = this->back_dependency.begin(); iter != this->back_dependency.end(); ++iter )
    {
        auto pair = *iter;
        void* powner = pair.first;
        int type = pair.second;

        if ( type == TYPE_FACTORY )
        {
            STREAMFACTORY* streamfactory = ( STREAMFACTORY* ) powner;
            if ( streamfactory->getSTREAMFACTORYState() == true )
            {
                printf ( "factory is running\n" );
                return false;
            }
        }
        else if ( type == TYPE_CELL )
        {
            CELL* cell = ( BASICCELL* ) powner;
            //printf("cellstate %d workerstate: %d\n", cell->getCELLState(), cell->getWORKERState());
            // 셀은 논러닝 하고 워커가 작동하는건 없어야 한다.
            if ( cell->getCELLState() || cell->getWORKERState() )
            {
                //printf ( "cell is running\n" );
                return false;
            }
        }
        else
        {
            printf ( "Unknown dependency type! %d\n", type );
            exit ( 0 );
        }
    }

    // 직렬화단계
    lockfreeq* q = this->getQueue();
    vector<char> bytearray;
    // packet full data와 fd를 넣는다.

    while ( !q->empty() )
    {
        TUPLE* tuple;
        q->pop ( tuple );
        // 0xAA
        bytearray.push_back ( 0xAA );
        // Size [2byte]
        // 사이즈는 현재 원래의 content에 총 8 (ip, queue number)바이트가 더 더해짐
        ushort size = tuple->getLen() + 8;
        //printf ( "migration size: %d\n", size );
        bytearray.insert ( bytearray.end(), ( char* ) &size, ( char* ) &size + 2 );

        // type
        bytearray.push_back ( tuple->gettype() );

        // [ip 4byte]
        int fd = tuple->getfd();
        // find ip by fd
        struct sockaddr_in* sain = (struct sockaddr_in*)(this->parent->getClientbyfd(fd)->var_sockaddr);
        unsigned int ip = sain->sin_addr.s_addr;
        //printf("ip: %d queid: %d\n", ip, id);
        bytearray.insert ( bytearray.end(), ( ( char* ) &ip ), ( ( char* ) &ip ) + 4 );
        // content [queue number 4byte]
        bytearray.insert ( bytearray.end(), ( ( char* ) &this->id ), ( ( char* ) &this->id ) + 4 );
        // real content
        bytearray.insert ( bytearray.end(), tuple->getcontent(), tuple->getcontent() + tuple->getLen() );
        //bytearray.insert(bytearray.end(), data->getdata(), data->getdata() + data->getLen());
    }
    
    //debug_packet(bytearray.data(), 100);
    
    vector<char> wrapper_bytearray;

    // 패킷 형태
    // 0xAA [size short] [type] [data] [data] [data] [data] ..
    // [data] = 0xAA [size short] [type] ([contents] == [fd 4byte] [queue number 4byte] [real contents])
    // [data]를 DATA형태로 변환 뒤, 큐에 집어넣으면 됨

    // 앞에 0xaa 프로토콜 사용 표시
    wrapper_bytearray.push_back ( 0xAA );

    // 2바이트는 short형태의 사이즈 표시
    ushort size = bytearray.size(); // size는 컨텐츠의 사이즈만 넣음
    char csize[2] = "";
    memcpy ( csize, &size, 2 );
    wrapper_bytearray.insert ( wrapper_bytearray.end(), csize, csize + 2 );

    // Type 1바이트
    wrapper_bytearray.push_back ( 0x02 );

    // 컨텐츠 바이트
    wrapper_bytearray.insert ( wrapper_bytearray.end(), bytearray.data(), bytearray.data() + bytearray.size() );

    //debug_packet ( wrapper_bytearray.data(), wrapper_bytearray.size() );

    // 보내기 단계
    int result = send ( fd, wrapper_bytearray.data(), wrapper_bytearray.size(), 0 );

    // 어레이 클리어
    bytearray.clear();
    wrapper_bytearray.clear();

    this->isMigrated = true;

    return true;
}

unsigned int PIPE::getid()
{
    return this->id;
}

void PIPE::install_comp_signal()
{
    // for all forward_dependency, 다 깨운다....
    for ( auto iter = this->forward_dependency.begin(); iter != this->forward_dependency.end(); ++iter )
    {
        void* powner = ( *iter ).first;
        int type = ( *iter ).second;

        if ( type == 0 )
        {
            STREAMFACTORY* streamfactory = ( STREAMFACTORY* ) powner;
            // 익스큐터에 연결되어있는 첫번째 태스크를 런 시킨다.
            CELL* cell = * ( streamfactory->getCELLs().begin() );
            if ( cell->getCELLState() == false )
            {
                cell->schedulingStart();
            }
        }
        else if ( type == 1 )
        {
            BASICCELL* cell = ( BASICCELL* ) powner;
            if ( cell->getCELLState() == false )
            {
                cell->schedulingStart();
            }

        }
        else if ( type == 2 )
        {
            // 깨울만한게 없음...
        }
        else
        {
            printf ( "Unknown dependency type! %d\n", type );
            exit ( 0 );
        }
    }
}

void PIPE::clearMigration()
{
    this->isMigrated = false;
}
