#include "Functions.h"
#include "Data.h"

DATA* task_1(DATA* data)
{
    int value = data->getInt();
    printf("value1: %d\n", value);
    
    
    value += 1;
    
    
    DATA* result = new DATA(4, data->getfd(), data->gettype());
    result->push(&value, 4);
    result->sealing();
    
    // 프로세싱
    sleep(2);
    
    // data 정리
    delete[] data->getdata();
    delete data;
    return result;
}

DATA* task_2(DATA* data)
{ 
    int value = data->getInt();
    printf("value2: %d\n", value);
    
    value += 1;
    DATA* result = new DATA(4, data->getfd(), data->gettype());
    result->push(&value, 4);
    result->sealing();
    
    // 프로세싱
    sleep(2);
    
    // data 정리
    delete[] data->getdata();
    delete data;
    return result;
}