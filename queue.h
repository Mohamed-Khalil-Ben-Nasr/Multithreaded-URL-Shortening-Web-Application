#include <semaphore.h>

#define QUEUE_SIZE 16

typedef struct {
    int d[QUEUE_SIZE];
    int front;
    int back;
    sem_t mutex; 
    sem_t slots; 
    sem_t items; 
} queue;

queue* queueCreate();
void enqueue(queue* q,int fd);
int dequeue(queue* q);