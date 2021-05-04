#ifndef HEADER_FILE
#define HEADER_FILE


typedef struct item {
  int type; //Machine type
  int time; //Using time
}Item;

typedef struct queue {
	int size;
    int len;
    int head;
    Item *buffer;
}Queue;

Queue* queue_init (int size);
int queue_destroy (Queue *q);
int queue_put (Queue *q, Item *item);
Item* queue_get(Queue *q);
int queue_empty (Queue *q);
int queue_full(Queue *q);

#endif
