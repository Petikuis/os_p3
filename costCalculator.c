
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <stddef.h>
#include <sys/stat.h>
#include <pthread.h>
#include "queue.h"
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>


#define NUM_CONSUMERS 1

// Create mutex valriables
/*************************************/
pthread_mutex_t buffer;
pthread_cond_t can_put;
pthread_cond_t can_get;

/*************************************/

// Create queue and total
/*************************************/
Queue *q;
int total = 0;

// Create structure variables
/*************************************/

enum{COMMON, COMPUTATION, SUPER};

int type_to_cost[] = {
	[COMMON] = 1,
	[COMPUTATION] = 3,
	[SUPER] = 10
};

typedef struct op_array{
	int len;
	Operation *ops;
}OpArray;

/*************************************/

//Parse the given file
/*************************************/
void apply_input_redirection(const char *filename){
	int fd_in;
	if ((fd_in = open(filename, O_RDONLY)) < 0){
        printf("[ERROR] Input file is not valid\n");
        exit(-1);
    }
    close(STDIN_FILENO);
    dup2(fd_in, STDIN_FILENO);
    close(fd_in);
}

int file_parser(const char *filename, int producer_num, OpArray *op_array){
	int op_num, previous;
	apply_input_redirection(filename);
	scanf("%d", &op_num);
	int assigned_ops = op_num / producer_num, 
		remainder_ops = op_num % producer_num;

	for(int producer = 0; producer < producer_num; producer++){
    	int ops_to_insert = assigned_ops + (remainder_ops > 0 ? 1 : 0);
    	remainder_ops--;
    	op_array[producer].len = ops_to_insert;
    	op_array[producer].ops = (Operation *)malloc(ops_to_insert * sizeof(Operation));
    	for(int operation = 0; operation < ops_to_insert; operation++){
    		int id, type, time;
        	scanf("%d %d %d", &id, &type, &time);
        	if (previous == id) return -1;
        	else previous = id;
        	(op_array[producer].ops)[operation].id = id;
        	(op_array[producer].ops)[operation].type = --type;
        	(op_array[producer].ops)[operation].time = time;
    	}
	}
	return op_num;

}

/*************************************/

//Producer and consumer functions
/*************************************/
void* produce(void* args){
	OpArray *prod_op_array = (OpArray *)args;
	int array_size = prod_op_array->len;
	Operation *prod_ops = prod_op_array->ops;

	for (int op = 0; op < array_size; op++){
		pthread_mutex_lock(&buffer);
		while (queue_full(q))
			pthread_cond_wait(&can_put, &buffer);
		queue_put(q, &prod_ops[op]);
		pthread_cond_signal(&can_get);
		pthread_mutex_unlock(&buffer);
	}
	pthread_exit(0);

}

void* consume(void* args){
	Operation *data;
	int remaining_ops = *((int *)args);

	while (remaining_ops > 0){
		pthread_mutex_lock(&buffer);
		while (queue_empty(q)){
			pthread_cond_wait(&can_get, &buffer);
		}
		data = queue_get(q);
		total += type_to_cost[data->type] * data->time;
		//printf("New consume with type %d, time %d, cost %d and total %d\n", data->type, data->time, type_to_cost[data->type] * data->time, total);
		pthread_cond_signal(&can_put);
		pthread_mutex_unlock(&buffer);
		remaining_ops--;
	}
	pthread_exit(0);

}

/**
 * Entry point
 * @param argc
 * @param argv
 * @return
 */
int main (int argc, const char * argv[] ) {

	if (argc > 4){
		perror("Number of args invalid");
		return -1;
	}

	int producer_num = atoi(argv[2]);
	if (producer_num <= 0){
		perror("Number of producers invalid");
		return -1;
	}

	int queue_size = atoi(argv[3]);
	if (queue_size <= 0){
		perror("Circular buffer size invalid");
		return -1;
	}

	q = queue_init(queue_size);
	OpArray op_array[producer_num];
	int op_num = file_parser(argv[1], producer_num, op_array);
	if (op_num < 1){
		perror("Not enough operations");
		return -1;
	}

	pthread_mutex_init(&buffer, NULL);
	pthread_cond_init(&can_put, NULL);
	pthread_cond_init(&can_get, NULL);

	pthread_t producers[producer_num];
	pthread_t consumer;

	pthread_create(&consumer, NULL, (void *)&consume, (void *)&op_num);

	for (int producer = 0; producer < producer_num; producer++){
		pthread_create(&producers[producer], NULL, (void *)&produce, (void *)&op_array[producer]);
	}

	for (int producer = 0; producer < producer_num; producer++){
		pthread_join(producers[producer], NULL);
	}

	pthread_join(consumer, NULL);

	printf("Total: %i €.\n", total);

	queue_destroy(q);
	pthread_mutex_destroy(&buffer);
	pthread_cond_destroy(&can_put);
	pthread_cond_destroy(&can_get);

	/*for (int producer = 0; producer < producer_num; producer++){
		int array_size = op_array[producer].len;
		for (int op = 0; op < array_size; op++){
			printf("Operation assigned to producer %d with id %d, type %d and time %d\n", 
					producer, (op_array[producer].ops)[op].id, 
					(op_array[producer].ops)[op].type, 
					(op_array[producer].ops)[op].time);
		}

	}



	Queue *q = queue_init(5);
	queue_put(q, op_init(0, COMMON, 3));
	queue_put(q, op_init(1, COMPUTATION, 5));
	queue_put(q, op_init(2, SUPER, 7));
	queue_put(q, op_init(3, COMMON, 7));
	queue_put(q, op_init(4, COMPUTATION, 7));

	while (!queue_empty(q)){
		Operation *op = queue_get(q);
		printf("The %d item has type %d and the time is %d\n", op->id, op->type, op->time);
		if (op->id == 0) queue_put(q, op_init(5, SUPER, 10));
	}

	queue_destroy(q);


    int total = 0;
    printf("Total: %i €.\n", total);*/

    return 0;
}
