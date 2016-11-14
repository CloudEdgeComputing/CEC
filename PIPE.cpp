#include "PIPE.h"
#include "BASICCELL.h"
#include "WORKER.h"
#include "STREAMFACTORY.h"
#include "TUPLE.h"
#include "STREAMFACTORY.h"
#include "Debug.h"

PIPE::PIPE ( quetype* queue, void* owner, int type, int id, STREAMFACTORY* parent )
{
    this->queue = queue;
    if ( owner != NULL )
    {
        this->back_dependency.push_back ( make_pair ( owner, type ) );
    }
    this->isMigrated = false;
    this->id = id;
    this->parent = parent;
    this->newest_tuple = NULL;
    this->ignored_tuple_uuid = 0;
}

quetype* PIPE::getQueue()
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

bool PIPE::sendQueue ( int fd, unsigned int uuid )
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
            // 셀의 워커 중 지정된 uuid를 처리하고 있는 워커가 없어야 한다.
            if ( cell->hasuuid(uuid) )
            {
                printf ( "cell is processing uuid: %d\n", uuid );
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
    //lockfreeq* q = this->getQueue();
    vector<char> bytearray;
    // packet full data와 fd를 넣는다.

    while ( !this->empty() )
    {
        TUPLE* tuple;
        tuple = this->pop ( );
        // 0xAA
        bytearray.push_back ( 0xAA );
        // Size [2byte]
        // 사이즈는 현재 원래의 content에 총 8 (ip, queue number)바이트가 더 더해짐
        ushort size = tuple->getLen() + 8;
        //printf ( "migration size: %d\n", size );
        bytearray.insert ( bytearray.end(), ( char* ) &size, ( char* ) &size + 2 );

        // type
        bytearray.push_back ( tuple->gettype() );

        // [uuid 4byte]
        int uuid = tuple->getuuid();
        // find ip by fd
        struct sockaddr_in* sain = (struct sockaddr_in*)(this->parent->getClientbyuuid(uuid)->var_sockaddr);
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

        if ( type == TYPE_FACTORY )
        {
            STREAMFACTORY* streamfactory = ( STREAMFACTORY* ) powner;
            // 익스큐터에 연결되어있는 첫번째 태스크를 런 시킨다.
            CELL* cell = * ( streamfactory->getCELLs().begin() );
            if ( cell->getCELLState() == false )
            {
                cell->schedulingStart();
            }
        }
        else if ( type == TYPE_CELL )
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

void PIPE::setnewest ( TUPLE* tuple )
{
    TUPLE* result = new TUPLE(tuple->getdata(), tuple->getLen(), 0, tuple->getuuid());
    this->newest_tuple = result;
}

TUPLE* PIPE::getnewest(bool copied)
{
    if(copied && newest_tuple != NULL)
    {
        TUPLE* result = new TUPLE(newest_tuple->getdata(), newest_tuple->getLen(), 0, newest_tuple->getuuid());
        return result;
    }
    return newest_tuple;
}

void PIPE::deletenewest()
{
    delete[] newest_tuple->getdata();
    delete newest_tuple;
}

void PIPE::push ( TUPLE* data )
{
    this->queue->push_back(data);
}

TUPLE* PIPE::pop()
{
    if(this->queue->empty() == false)
    {
        if(this->ignored_tuple_uuid == 0)
        {
            TUPLE* tuple = this->queue->front();
            this->queue->pop_back();
            return tuple;
        }
        else
        {
            for(auto iter = this->queue->begin(); iter != this->queue->end(); ++iter)
            {
                TUPLE* tuple = *iter;
                if(tuple->getuuid() != this->ignored_tuple_uuid)
                {
                    this->queue->erase(iter);
                    return tuple;
                }
            }
        }
    }
    return NULL;
}

bool PIPE::empty()
{
    if(this->ignored_tuple_uuid == 0)
        return this->queue->empty();
    else
    {
        for(auto iter = this->queue->begin(); iter != this->queue->end(); ++iter)
        {
            TUPLE* tuple = *iter;
            if(tuple->getuuid() == this->ignored_tuple_uuid)
                return false;
        }
        return true;
    }
    return false;
}

void PIPE::setingnoreuuid ( unsigned int uuid )
{
    this->ignored_tuple_uuid = uuid;
}

void PIPE::clearignoreuuid()
{
    this->ignored_tuple_uuid = 0;
}