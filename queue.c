#include "queue.h"
#include <stdlib.h>

queue* queueCreate() {
    queue *q = (queue*) malloc(sizeof(queue));
    q->front = 0;
    q->back = 0;
    sem_init(&q->mutex,0,1);
    sem_init(&q->slots,0,QUEUE_SIZE);
    sem_init(&q->items,0,0);
    return q;
}

void enqueue(queue* q,int fd) {
    sem_wait(&q->slots);
    sem_wait(&q->mutex);
    q->d[q->back] = fd;
    q->back = (q->back+1)%QUEUE_SIZE;
    sem_post(&q->mutex);
    sem_post(&q->items);
}

int dequeue(queue* q) {
    int fd;
    sem_wait(&q->items);
    sem_wait(&q->mutex);
    fd = q->d[q->front];
    q->front = (q->front+1)%QUEUE_SIZE;
    sem_post(&q->mutex);
    sem_post(&q->slots);
    return fd;
}