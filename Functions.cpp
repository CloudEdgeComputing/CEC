#include "Functions.h"
#include "TUPLE.h"
#include "STATEMANAGER.h"
#include <math.h>
#include "Debug.h"

int zerodelta = 0;
int backup = 0;
int stepcnt = 0;

TUPLE* pedtask(TUPLE* data, STATEMANAGER* statemanager)
{
    int pedstr = data->getInt();

    float pedexp = (27.55 - (20 * log10((unsigned long)2412)) + abs(pedstr))/20;
    float peddist = pow10(pedexp);
    DBCONN* conn = statemanager->getconn(data->getuuid());
    
    statemanager->writeTable(conn, "info", 1, peddist, 1);
    return NULL;
}

TUPLE* cartask(TUPLE* data, STATEMANAGER* statemanager)
{
    int carstr = data->getInt();
    
    printf("carvalue: %d\n", carstr);
    
    DBCONN* conn = statemanager->getconn(data->getuuid());
    
    TUPLE* readdata = statemanager->readTable(conn, "info", 1);
    
    float peddist = 1;
    if(readdata != NULL)
    {
        readdata->getFloat();
        peddist = (float)readdata->getFloat();
        
        delete[] readdata->getdata();
        delete readdata;
    }
    
    double carexp = (27.55 - (20 * log10((unsigned long)2412)) + abs(carstr))/20;
    double cardist = pow10(carexp);
    
    TUPLE* result = new TUPLE(4, data->getuuid(), data->gettype());
    
    printf("cardistance: %f peddist: %f\n", cardist, peddist);
    
    bool carin = false;
    bool pedin = false;
    if(peddist < 0.25)
    {
        pedin = true;
    }
    
    int value = -1;
    // 총 세번의 threshold를 친 경우 인정한다.
    // 첫번째 치고 변화가 세번째 없는경우
    static bool sw = false;
    
    if(cardist < 0.25)
    {
        stepcnt++;
    }
    
    if(backup == stepcnt)
    {
        zerodelta++;
    }
    
    backup = stepcnt;
    
    if(zerodelta > 2)
    {
        stepcnt = 0;
        backup = 0;
        zerodelta = 0;
    }
    
    printf("zerodelta: %d backup: %d stepcnt: %d\n", zerodelta, backup, stepcnt);
    
    if(stepcnt > 2)
    {
        carin = true;
    }
    
    if(carin && pedin)
    {
        stepcnt = 0;
        zerodelta = 0;
        backup = 0;
        value = 1;
    }
    else
        value = 0;
    
    result->push(&value, 4);
    result->sealing();
    
    // data 정리
    delete[] data->getdata();
    delete data;
    
    return result;
}
