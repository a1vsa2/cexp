
#include "xqueue.h"
#include <windows.h>

HANDLE gMutex;
static long glimitSize;
static int stop = 0;

Queue* createQueue(int size) {
    Queue* q = (Queue*)malloc(sizeof(Queue));
    if (!q) return NULL;
    q->front = q->rear = NULL;
    q->size = 0;
    stop = 0;
    glimitSize = size <= 0 ? INT_MAX : size;
    InitializeCriticalSection(&q->lock);
    InitializeConditionVariable(&q->notEmpty);
    return q;
}

void enqueue(Queue* q, void* data, int len) {
    if (!q) return;

    QueueNode* newNode = (QueueNode*)malloc(sizeof(QueueNode));
    if (!newNode) return;

    newNode->data = data;
    newNode->len = len;
    newNode->next = NULL;
    EnterCriticalSection(&q->lock);
    while(q->size >= glimitSize && !stop) {
        // stolen wake-up: 条件变量触发后会阻塞直到获取到锁，在此期间数据状态可能已被另外线程改变
        SleepConditionVariableCS(&q->notFull, &q->lock, INFINITE);
        // if (q->size == glimitSize) {
        //     printf("#### enqueue: spurious wake\n");
        // }
    }
    if (stop) {
        free(newNode);
        LeaveCriticalSection(&q->lock);
        return;
    }
    if (q->rear == NULL) {
        q->front = q->rear = newNode;
    } else {
        q->rear->next = newNode;
        q->rear = newNode;
    }
    q->size++;
    LeaveCriticalSection(&q->lock);
    WakeConditionVariable(&q->notEmpty);
}

void* dequeue(Queue* q, int* len) {
    if (!q) {
        return NULL;
    }
    EnterCriticalSection(&q->lock);
    while(q->size == 0 && !stop) {
        SleepConditionVariableCS(&q->notEmpty, &q->lock, INFINITE);
    }
    if (stop) {
        LeaveCriticalSection(&q->lock);
        return NULL;
    }
    QueueNode* temp = q->front;
    void* data = temp->data;
    // *len = temp->len;
    q->front = q->front->next;
    if (q->front == NULL) 
        q->rear = NULL;

    q->size--;
    LeaveCriticalSection(&q->lock);
    WakeConditionVariable(&q->notFull);
    free(temp);
    return data;
}

void destroyQueue(Queue* q) {
    stop = 1;
    EnterCriticalSection(&q->lock);
    WakeAllConditionVariable (&q->notEmpty);
    WakeAllConditionVariable (&q->notFull);
    while (q->front != NULL) {
        void* data = q->front;
        q->front = q->front->next;
        free(q->front);
    }
    free(q);
    LeaveCriticalSection(&q->lock); 
    DeleteCriticalSection(&q->lock);
    q = NULL;
}