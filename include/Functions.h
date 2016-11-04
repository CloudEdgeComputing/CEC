#pragma once
#include <list>
#include "Type.h"

using namespace std;

class TUPLE;

typedef TUPLE* (*FUNC)(TUPLE*);
typedef TUPLE* (*MERGE_FUNC)(list<TUPLE*>*, uint dominantpipeid);

TUPLE* task_1(TUPLE* data);
TUPLE* task_2(TUPLE* data);