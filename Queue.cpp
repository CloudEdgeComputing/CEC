#include "Queue.h"
#include "Task.h"
#include "Operator.h"
#include "Executor.h"
#include "Connection.h"
#include "Data.h"

QUEUE::QUEUE( lockfreeq* queue, void* owner, int type )
{
    this->queue = queue;
    this->owner.push_back(make_pair(owner, type));
    this->isMigrated = false;
}

boost::lockfree::queue< DATA* >* QUEUE::getQueue()
{
    return this->queue;
}

void QUEUE::registerDependency ( void* owner, int type )
{
    this->owner.push_back(make_pair(owner, type));
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
    for(auto iter = this->owner.begin(); iter != this->owner.end(); ++iter)
    {
        auto pair = *iter;
        void* powner = pair.first;
        int type = pair.second;
        
        if(type == 0)
        {
            Executor* executor = (Executor*)powner;
            if(executor->getExecutorState() == true)
                return false;
        }
        else if(type == 1)
        {
            Task* task = (Task*)powner;
            if(task->getTaskState() == true)
                return false;
            
        }
        else if(type == 2)
        {
            Connection* conn = (Connection*)powner;
            if(conn->getConnState() == true)
                return false;
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
        bytearray.insert(bytearray.end(), data->getdata(), data->getdata() + data->getLen());
        char cfd[4] = "";
        memcpy(cfd, &fd, 4);
        bytearray.insert(bytearray.end(), cfd, 4);
    }
    
    vector<char> wrapper_bytearray;
    
    // 앞에 0xaa 프로토콜 사용 표시
    wrapper_bytearray.push_back(0xAA);
    
    // 2바이트는 short형태의 사이즈 표시
    ushort size = bytearray.size();
    char csize[2] = "";
    memcpy(csize, &size, 2);
    wrapper_bytearray.insert(wrapper_bytearray.end(), csize, csize + 2);
    
    // Type 1바이트 
    wrapper_bytearray.push_back(0x02);
    
    // 컨텐츠 바이트
    wrapper_bytearray.insert(wrapper_bytearray.end(), bytearray.data(), bytearray.data() + bytearray.size());
    
    // 보내기 단계
    send(fd, wrapper_bytearray.data(), 0);
    bytearray.clear();
    wrapper_bytearray.clear();
    
    this->isMigrated = true;
    
    return true;
}