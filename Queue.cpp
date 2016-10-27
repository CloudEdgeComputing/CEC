#include "Queue.h"
#include "Task.h"
#include "Operator.h"

QUEUE::QUEUE( lockfreeq* queue, void* owner )
{
    this->queue = queue;
    this->owner = owner;
}

boost::lockfree::queue< DATA* >* QUEUE::getQueue()
{
    return this->queue;
}