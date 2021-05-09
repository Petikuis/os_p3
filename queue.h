#ifndef HEADER_FILE
#define HEADER_FILE

typedef struct operation {
    int type; // Machine type
    int time; // Using time
}Operation;

typedef struct queue {
    int size;
    int len;
    int head;
    Operation *buffer;
}Queue;

Queue* queue_init (int size);
int queue_destroy (Queue *q);
int queue_put (Queue *q, Operation *item);
Operation* queue_get(Queue *q);
int queue_empty (Queue *q);
int queue_full(Queue *q);

#endif
