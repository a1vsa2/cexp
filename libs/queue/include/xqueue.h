#ifndef __XQUEUE_H__
#define __XQUEUE_H__

struct S_NODE;

typedef struct S_NODE {
    void* val;
    int len;
    Node* next;
} Node;

typedef enum {
    FALSE,
    TRUE,
} BOOLEAN;

typedef struct S_QUEUE {
    int size;
    Node *head;
    Node *tail;
} XQUEUE;


XQUEUE* createQueue();
int destroyQueue(XQUEUE*);
BOOLEAN xqueue_put(void*, int);
void* xqueue_get();

#endif // __XQUEUE_H__