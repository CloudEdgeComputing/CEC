#pragma once
#include <list>
#include "Type.h"

using namespace std;

class TUPLE;
class STATEMANAGER;

typedef TUPLE* (*FUNC)(TUPLE*, STATEMANAGER*);
typedef TUPLE* (*MERGE_FUNC)(list<TUPLE*>*, uint dominantpipeid);

TUPLE* cartask(TUPLE* data, STATEMANAGER* statemanager);
TUPLE* pedtask(TUPLE* data, STATEMANAGER* statemanager);