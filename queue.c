


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
    q->buffer = (Item *)malloc(size * sizeof(Item)); 

    return q;
}


// To Enqueue an element
int queue_put(Queue *q, Item *x) {
    if(queue_full(q))
        return -1;
    int index = (q->head + q->len) % q->size;
    q->len++;
    q->buffer[index] = *x;
    free(x); // Lose referenciation to the item 
    return 0;
}


// To Dequeue an element.
Item* queue_get(Queue *q) {
    if(queue_empty(q))
        return NULL;
    Item* item = &(q->buffer[q->head]);
    q->head = (q->head + 1) % q->size;
    q->len--;
    return item;
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


Item* item_init(int type, int time){

    Item *i = (Item *)malloc(sizeof(Item));
    i->type = type;
    i->time = time;

    return i;
}