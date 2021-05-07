#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include "queue.h"



//To create a queue
Queue* queue_init(int size){

    Queue *q = (Queue *)malloc(sizeof(Queue));
    q->size = size;
    q->len = 0;
    q->head = 0;
    q->buffer = (Operation *)malloc(size * sizeof(Operation)); 

    return q;
}


// To Enqueue an element
int queue_put(Queue *q, Operation *x) {
    if(queue_full(q))
        return -1;
    int index = (q->head + q->len) % q->size;
    q->len++;
    q->buffer[index] = *x;
    return 0;
}


// To Dequeue an element.
Operation* queue_get(Queue *q) {
    if(queue_empty(q))
        return NULL;
    Operation* op = &(q->buffer[q->head]);
    q->head = (q->head + 1) % q->size;
    q->len--;
    return op;
}


//To check queue state
int queue_empty(Queue *q){
    return q->len == 0 ? 1 : 0;
}

int queue_full(Queue *q){
    return q->len == q->size ? 1 : 0;
}

//To destroy the queue and free the resources
int queue_destroy(Queue *q){
    free(q);
    return 0;
}


Operation* op_init(int id, int type, int time){

    Operation *op = (Operation *)malloc(sizeof(Operation));
    op->id = id;
    op->type = type;
    op->time = time;

    return op;
}