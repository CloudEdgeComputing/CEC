#include "Queue.h"
#include "Task.h"
#include "Operator.h"
#include "Executor.h"
#include "Connection.h"
#include "Data.h"
#include "Debug.h"

QUEUE::QUEUE( lockfreeq* queue, void* owner, int type, int id)
{
    this->queue = queue;
    if(owner != NULL)
        this->back_dependency.push_back(make_pair(owner, type));
    this->isMigrated = false;
    this->id = id;
}

boost::lockfree::queue< DATA* >* QUEUE::getQueue()
{
    return this->queue;
}

void QUEUE::registerbackDependency ( void* owner, int type )
{
    this->back_dependency.push_back(make_pair(owner, type));
}

void QUEUE::registerforwardDependency ( void* owner, int type )
{
    this->forward_dependency.push_back(make_pair(owner, type));
}

bool QUEUE::sendQueue ( int fd )
{
    if(this->isMigrated == true)
    {
        return true;
    }
    
    // Queue의 Dependency 체크
    // 모든 Dependency를 가진 것들이 멈춰야 한다.
    // 안멈춰 있으면 False return
    for(auto iter = this->back_dependency.begin(); iter != this->back_dependency.end(); ++iter)
    {
        auto pair = *iter;
        void* powner = pair.first;
        int type = pair.second;
        
        if(type == 0)
        {
            Executor* executor = (Executor*)powner;
            if(executor->getExecutorState() == true)
            {
                printf("executor state is running\n");
                return false;
            }
        }
        else if(type == 1)
        {
            Task* task = (Task*)powner;
            if(task->getTaskState() == true)
            {
                printf("Task state is running\n");
                return false;
            }
            
        }
        else if(type == 2)
        {
            Connection* conn = (Connection*)powner;
            if(conn->getConnState() == true)
            {
                printf("Connection state is running\n");
                return false;
            }
        }
        else
        {
            printf("Unknown dependency type! %d\n", type);
            exit(0);
        }
    }
    
    // 직렬화단계 
    lockfreeq* q = this->getQueue();
    vector<char> bytearray;
    // packet full data와 fd를 넣는다.
    while(!q->empty())
    {
        DATA* data;
        q->pop(data);
        char cfd[4] = "";
        memcpy(cfd, &fd, 4);
        // 0xAA
        bytearray.push_back(0xAA);
        // Size [2byte]
        // 사이즈는 현재 원래의 content에 총 8바이트가 더 더해짐
        ushort size = data->getLen() + 8;
        printf("size: %d\n", size);
        bytearray.insert(bytearray.end(), (char*)&size, (char*)&size + 2);
        
        // type
        bytearray.push_back(data->gettype());

        // content [fd 4byte]
        int fd = data->getfd();
        bytearray.insert(bytearray.end(), ((char*)&fd), ((char*)&fd) + 4);
        // content [queue number 4byte]
        bytearray.insert(bytearray.end(), ((char*)&this->id), ((char*)&this->id) + 4);
        // real content
        bytearray.insert(bytearray.end(), data->getcontent(), data->getcontent() + data->getLen());
        //bytearray.insert(bytearray.end(), data->getdata(), data->getdata() + data->getLen());
    }
    
    vector<char> wrapper_bytearray;
    
    // 패킷 형태 
    // 0xAA [size short] [type] [data] [data] [data] [data] ..
    // [data] = 0xAA [size short] [type] ([contents] == [fd 4byte] [queue number 2byte] [real contents]) 
    // [data]를 DATA형태로 변환 뒤, 큐에 집어넣으면 됨
    
    // 앞에 0xaa 프로토콜 사용 표시
    wrapper_bytearray.push_back(0xAA);
    
    // 2바이트는 short형태의 사이즈 표시
    ushort size = bytearray.size(); // size는 컨텐츠의 사이즈만 넣음
    char csize[2] = "";
    memcpy(csize, &size, 2);
    wrapper_bytearray.insert(wrapper_bytearray.end(), csize, csize + 2);
    
    // Type 1바이트 
    wrapper_bytearray.push_back(0x02);
    
    // 컨텐츠 바이트
    wrapper_bytearray.insert(wrapper_bytearray.end(), bytearray.data(), bytearray.data() + bytearray.size());
    
    debug_packet(wrapper_bytearray.data(), wrapper_bytearray.size());
    
    // 보내기 단계
    int result = send(fd, wrapper_bytearray.data(), wrapper_bytearray.size(), 0);
    printf("send size: %d\n", result);
    // 어레이 클리어
    bytearray.clear();
    wrapper_bytearray.clear();
    
    this->isMigrated = true;
    
    return true;
}

unsigned int QUEUE::getid()
{
    return this->id;
}

void QUEUE::install_comp_signal()
{
    // for all forward_dependency, 다 깨운다....
    for(auto iter = this->forward_dependency.begin(); iter != this->forward_dependency.end(); ++iter)
    {
        void* powner = (*iter).first;
        int type = (*iter).second;
        
        if(type == 0)
        {
            Executor* executor = (Executor*)powner;
            // 익스큐터에 연결되어있는 첫번째 태스크를 런 시킨다.
            Task* task = *(executor->getTask().begin());
            if(task->getTaskState() == false)
                task->SchedulingStart();
        }
        else if(type == 1)
        {
            Task* task = (Task*)powner;
            if(task->getTaskState() == false)
                task->SchedulingStart();
            
        }
        else if(type == 2)
        {
            // 깨울만한게 없음...
        }
        else
        {
            printf("Unknown dependency type! %d\n", type);
            exit(0);
        }
    }
}

void QUEUE::clearMigration()
{
    this->isMigrated = false;
}