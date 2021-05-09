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

/*************************************/

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
        perror("[ERROR] Input file is not valid");
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
        	if (previous == id) exit(-1);
        	else previous = id;
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
		if(pthread_mutex_lock(&buffer) < 0){
			perror("[Error]: Could not lock the mutex");
			exit(-1);
		}
		while (queue_full(q)){
			if(pthread_cond_wait(&can_put, &buffer) < 0){
				perror("[Error]: Could not wait for condition");
				exit(-1);
			}
		}
		if(queue_put(q, &prod_ops[op])< 0){
			perror("[Error]: The queue is empty");
			exit(-1);
		}
		if(pthread_cond_signal(&can_get)< 0){
			perror("[Error]: Could not post for condition");
			exit(-1);
		}
		if(pthread_mutex_unlock(&buffer) < 0){
			perror("[Error]: Could not unlock the mutex");
			exit(-1);
		}
	}
	pthread_exit(0);

}

void* consume(void* args){
	Operation *data;
	int remaining_ops = *((int *)args);

	while (remaining_ops > 0){
		if(pthread_mutex_lock(&buffer) < 0){
			perror("[Error]: Could not lock the mutex");
			exit(-1);
		}
		while (queue_empty(q)){
			if(pthread_cond_wait(&can_get, &buffer) < 0){
				perror("[Error]: Could not wait for condition");
				exit(-1);
			}
		}
		data = queue_get(q);
		if(data < 0){
			perror("[Error]: Could not get an element from the queue");
			exit(-1);
		}
		total += type_to_cost[data->type] * data->time;
		//printf("New consume with type %d, time %d, cost %d and total %d\n", data->type, data->time, type_to_cost[data->type] * data->time, total);
		if(pthread_cond_signal(&can_put)< 0){
			perror("[Error]: Could not post for condition");
			exit(-1);
		}
		if(pthread_mutex_unlock(&buffer) < 0){
			perror("[Error]: Could not unlock the mutex");
			exit(-1);
		}
		remaining_ops--;
	}
	pthread_exit(0);

}

/*************************************/

// Get the number of ops that each consumer has to consume
/*************************************/
int get_ops_to_consume(int op_num, int consumer_num, int total_consumers){
    int base_ops = op_num / total_consumers,
        remainder_ops = op_num % total_consumers;
    return base_ops + (consumer_num < remainder_ops ? 1 : 0);
}

/*************************************/

int main (int argc, const char * argv[] ) {

	if (argc > 4){
		perror("Number of args invalid");
		exit(-1);
	}

	int producer_num = atoi(argv[2]);
	if (producer_num <= 0){
		perror("Number of producers invalid");
		exit(-1);
	}

	int queue_size = atoi(argv[3]);
	if (queue_size <= 0){
		perror("Circular buffer size invalid");
		exit(-1);
	}

	q = queue_init(queue_size);
	
	OpArray op_array[producer_num];
	int op_num = file_parser(argv[1], producer_num, op_array);
	if (op_num < 1){
		perror("Not enough operations");
		exit(-1);
	}

	if(pthread_mutex_init(&buffer, NULL) < 0){
		perror("[Error]: Could not init the mutex");
		exit(-1);
	}
	if(pthread_cond_init(&can_put, NULL) < 0){
		perror("[Error]: Could not init the condition");
		exit(-1);
	}
	if(pthread_cond_init(&can_get, NULL) < 0){
		perror("[Error]: Could not init the condition");
		exit(-1);
	}

	pthread_t producers[producer_num];
	pthread_t consumers[NUM_CONSUMERS];

	int assigned_ops = op_num / NUM_CONSUMERS,
        remainder_ops = op_num % NUM_CONSUMERS;

	for (int consumer = 0; consumer < NUM_CONSUMERS; consumer++){
		int ops_to_consume = assigned_ops + (remainder_ops > 0 ? 1 : 0);
		if(pthread_create(&consumers[consumer], NULL, (void *)&consume, (void *)&ops_to_consume) < 0){
			perror("[Error]: Could not create the consumer thread(s)");
			exit(-1);
		}
	}

	for (int producer = 0; producer < producer_num; producer++){
		if(pthread_create(&producers[producer], NULL, (void *)&produce, (void *)&op_array[producer]) < 0){
			perror("[Error]: Could not create the producer thread(s)");
			exit(-1);
		}
	}

	for (int producer = 0; producer < producer_num; producer++){
		if(pthread_join(producers[producer], NULL) < 0){
			perror("[Error]: Could not join the thread(s)");
			exit(-1);
		}
	}

	for (int consumer = 0; consumer < NUM_CONSUMERS; consumer++){
		if(pthread_join(consumers[consumer], NULL) < 0){
			perror("[Error]: Could not join the thread(s)");
			exit(-1);
		}
	}

	printf("Total: %i €.\n", total);

	queue_destroy(q);
	if(pthread_mutex_destroy(&buffer) < 0){
		perror("[Error]: Could not destroy the mutex");
		exit(-1);
	}
	if(pthread_cond_destroy(&can_put) < 0){
		perror("[Error]: Could not destroy the condition");
		exit(-1);
	}
	if(pthread_cond_destroy(&can_get) < 0){
		perror("[Error]: Could not destroy the condition");
		exit(-1);
	}

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
