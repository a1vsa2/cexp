#include "./include/xqueue.h"
#include <stdlib.h>

XQUEUE* createQueue() {
    XQUEUE* q = malloc(sizeof(XQUEUE));
    return q;
}

BOOLEAN xqueue_put(void* data, int len) {

}