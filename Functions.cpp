#include "Functions.h"
#include "TUPLE.h"

TUPLE* task_1(TUPLE* data)
{
    int value = data->getInt();
    
    value += 1;
    
    
    TUPLE* result = new TUPLE(4, data->getfd(), data->gettype());
    result->push(&value, 4);
    result->sealing();
    
    // 프로세싱
    sleep(2);
    
    // data 정리
    delete[] data->getdata();
    delete data;
    return result;
}

TUPLE* task_2(TUPLE* data)
{ 
    int value = data->getInt();
        
    value += 1;
    TUPLE* result = new TUPLE(4, data->getfd(), data->gettype());
    result->push(&value, 4);
    result->sealing();
    
    printf("value: %d from value: %d \n", value, value-2);
    
    // 프로세싱
    sleep(3);
    
    // data 정리
    delete[] data->getdata();
    delete data;
    return result;
}