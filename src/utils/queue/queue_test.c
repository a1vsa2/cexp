#include <string.h>

#include "xqueue.h"

// struct tpp {
//     Queue *q;
//     int tid;
// };

Queue* q;
long producerCount;
long consumerCount;
long producerVal;
ULONG WINAPI tProducer(LPVOID lpParam) {
    int id = *(int*)lpParam;
    while(1) {
        // Sleep(rand() % 200);
        char* cc = calloc(12, 1);
        InterlockedIncrement(&producerCount);
        producerVal++;
        sprintf(cc, "producer%d-%d", id, producerVal);
        enqueue(q, cc, 0);
    }
    printf("producer end\n");

}

ULONG WINAPI tConsumer(LPVOID lpParam) {
    int id = *((int*)lpParam);
    while(1) {    
        Sleep(rand() % 1000);
        char* cs = dequeue(q, &id);
        InterlockedIncrement(&consumerCount);
        // printf("consumer-%d: %s\n", id, cs);
        free(cs);
    }
    printf("consuemer end\n");
}

ULONG stolenWokenTest(LPVOID lpParam) {
    EnterCriticalSection(&q->lock);
    printf("thread wait variable\n");
    SleepConditionVariableCS(&q->notEmpty, &q->lock, INFINITE);
    LeaveCriticalSection(&q->lock);
    printf("thread1 release lock\n");
}

int main() {

    q = createQueue(2);
    // HANDLE th = CreateThread(0, 0, stolenWokenTest, 0, 0, 0);
    // Sleep(2000);
    // EnterCriticalSection(&q->lock);
    // WakeConditionVariable(&q->notEmpty);
    // Sleep(2000);
    // LeaveCriticalSection(&q->lock);
    // WaitForSingleObject(th, INFINITE);
    // CloseHandle(th);
    // return 0;

    int pn = 1;
    int cn = 2;
    HANDLE threads[pn + cn];
    int tids[pn+cn];
    for (int i = 0; i < pn; i++) {
        tids[i] = i+1;
        threads[i] = CreateThread(0, 0, tProducer, (PVOID)&tids[i], 0, 0);
    }
    
    for (int i = 0; i < cn; i++) {
        tids[pn + i] = i + 1;
        threads[pn + i] = CreateThread(0, 0, tConsumer, (PVOID)&tids[pn + i], 0, 0);
    }
    
    int eno = WaitForMultipleObjects(pn+cn, threads, TRUE, INFINITE);
    if (eno == -1) eno = GetLastError();
    unsigned long ecode = STILL_ACTIVE;
    for (int i = 0; i < pn + cn; i++) {
        printf("wait end %d\n", GetExitCodeThread(threads[i], &ecode));
        TerminateThread(threads[i], 0);
    }
    printf("%d, %d, %d %d\n", producerCount, consumerCount, q->size, eno);
    destroyQueue(q);
    return 0;
}