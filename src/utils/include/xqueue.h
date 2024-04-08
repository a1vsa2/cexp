#ifndef __XQUEUE_H__
#define __XQUEUE_H__

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>

#define _QUEUE_BLOCKING

typedef struct QueueNode {
    void* data;
    int len;
    struct QueueNode* next;
} QueueNode;

typedef struct {
    QueueNode* front;
    QueueNode* rear;
    volatile int size;
#ifdef _QUEUE_BLOCKING
    CRITICAL_SECTION lock;
    CONDITION_VARIABLE notEmpty;
    CONDITION_VARIABLE notFull;
#endif
} Queue;


Queue* createQueue(int size);
void enqueue(Queue* q, void* data, int len);
void* dequeue(Queue* q, int* len);
void destroyQueue(Queue* q);

#endif // __XQUEUE_H__