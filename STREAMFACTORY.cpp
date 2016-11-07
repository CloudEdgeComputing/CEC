#include "STREAMFACTORY.h"
#include "BASICCELL.h"
#include "PIPE.h"
#include "TUPLE.h"
#include "FACTORYBUILDER.h"
#include "Debug.h"

STREAMFACTORY::STREAMFACTORY()
{
}

void STREAMFACTORY::factoryStart()
{
    printf ( "For an factory, registered cell: %d\n", this->cells.size() );
    for ( auto iter = this->cells.begin(); iter != this->cells.end(); ++iter )
    {
        CELL* cell = *iter;
        printf ( "cell %p is started!\n", cell );
        cell->schedulingStart();
    }
}

list< CELL* > STREAMFACTORY::getCELLs()
{
    return this->cells;
}

void STREAMFACTORY::registerCELL ( CELL* cell )
{
    this->cells.push_back ( cell );
}

bool STREAMFACTORY::getSTREAMFACTORYState()
{
    return this->state;
}

bool STREAMFACTORY::setSTREAMFACTORYState ( bool state )
{
    this->state = state;
}

void STREAMFACTORY::installReceivedData ( TUPLE* data )
{
    if ( !data->validity() )
    {
        //debug_packet(data->getdata(), data->getLen() + 4);
        printf ( "Migrated tuple is not valid!\n" );
        return;
    }

    // 데이터 분해
    char* p = data->getcontent();
    ushort size = 0;

    // 오직 한번에 하나의 큐 데이터만 들어옴
    PIPE* pipe = NULL;

    if ( data->getLen() != 0 )
    {

        // p 4byte protocol  4byte ip 4byte queue real contents ...
        // 단위 데이터를 읽어 설치함
        while ( size < data->getLen() )
        {
            // 큐 아이디를 얻어냄
            unsigned int pipeid = 0;
            memcpy ( &pipeid, p + 8, 4 );
            
            // ip로 fd를 얻어냄
            unsigned int ip = 0;
            memcpy(&ip, p + 4, 4);
            int fd = this->getClientbyip(ip)->fd;        

            // unit data가 스페셜 형식을 가짐
            TUPLE* unitdata = new TUPLE ( p, true );
            
            unitdata->setfd(fd);
            
            printf ( "queue id: %d\n", pipeid );
            //printf("received packet!\n");
            //debug_packet(unitdata->getdata(), unitdata->getLen() + 4);

            // qid를 이용해 queue를 얻어옴
            pipe = factorymanager.findPIPE ( pipeid );

            // queue에 데이터를 푸시함
            pipe->getQueue()->push ( unitdata );

            p = p + unitdata->getLen() + 8 + 4;
            size += unitdata->getLen() + 8 + 4;
            printf("size: %d Len: %d\n", size, data->getLen());
            //printf("unitdata->getLen() + 8 + 4: %d\n", unitdata->getLen() + 8 + 4);
        }

        pipe->install_comp_signal();
    }
}

list< CLIENT* >* STREAMFACTORY::getsrcdevices()
{
    return &this->devices;
}

CLIENT* STREAMFACTORY::getClientbyip ( unsigned int ip )
{
    for(auto iter = this->devices.begin(); iter != this->devices.end(); ++iter)
    {
        CLIENT* client = *iter;
        struct sockaddr_in* sain = (sockaddr_in*)client->var_sockaddr;
        unsigned int _ip = sain->sin_addr.s_addr;
        if(_ip == ip)
            return client;
    }
}

CLIENT* STREAMFACTORY::getClientbyfd ( unsigned int fd )
{
    for(auto iter = this->devices.begin(); iter != this->devices.end(); ++iter)
    {
        CLIENT* client = *iter;
        unsigned int _fd = client->fd;
        if(fd == fd)
            return client;
    }
}

void STREAMFACTORY::printtasks()
{
    for ( auto iter = this->cells.begin(); iter != this->cells.end(); ++iter )
    {
        CELL* cell = *iter;
        printf ( "cell: %p\n", cell );
    }
    printf ( "end!\n" );
}
