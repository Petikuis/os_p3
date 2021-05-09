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

// Define structs
/*************************************/
typedef struct op_array{
	int len;
	Operation *ops;
}OpArray;
/*************************************/


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

// Create shortcut array
/*************************************/
enum{COMMON, COMPUTATION, SUPER};

int type_to_cost[] = {
	[COMMON] = 1,
	[COMPUTATION] = 3,
	[SUPER] = 10
};
/*************************************/

// Parse the given file
/*************************************/
void apply_input_redirection(const char *filename){
	int fd_in;
	// Try to open the file, close the program on failure
	if ((fd_in = open(filename, O_RDONLY)) < 0){
        perror("[Error] Input file is not valid");
        exit(-1);
    }
	// Close current STD_IN and replace with file
    close(STDIN_FILENO);
    dup2(fd_in, STDIN_FILENO);
    close(fd_in);
}

// This method assumes that the syntax of the input file is correct
int file_parser(const char *filename, int producer_num, OpArray *op_array){
	int op_num, previous;
	apply_input_redirection(filename);
	// Get the number of expected operations
	scanf("%d", &op_num);
	int assigned_ops = op_num / producer_num, 
		remainder_ops = op_num % producer_num;

	// There is no need to keep a global counter of how many operations have been inserted
	// since the sum of ops_to_insert will always be equal to op_num
	for(int producer = 0; producer < producer_num; producer++){

		// If there are unassigned operations, add one operation to the current producer
    	int ops_to_insert = assigned_ops + (remainder_ops > 0 ? 1 : 0);
    	remainder_ops--;

		// Store the size of this producer's array and reserve memory space for that size
    	op_array[producer].len = ops_to_insert;
    	op_array[producer].ops = (Operation *)malloc(ops_to_insert * sizeof(Operation));
    	for(int operation = 0; operation < ops_to_insert; operation++){
    		int id, type, time;
        	scanf("%d %d %d", &id, &type, &time);
			// scanf will return the previous value if it is unable to read further.
			// Since ids are unique, the verification can be done using them
        	if (previous == id){
				perror("[Error] Less operations than expected");
				exit(-1);
			}
        	else previous = id;

			// Assign values to the reserved memory space for this operation
        	(op_array[producer].ops)[operation].type = --type;
        	(op_array[producer].ops)[operation].time = time;
    	}
	}
	return op_num;

}
/*************************************/

// Producer and consumer functions
/*************************************/
void* produce(void* args){
	OpArray *prod_op_array = (OpArray *)args;
	int array_size = prod_op_array->len;
	Operation *prod_ops = prod_op_array->ops;

	for (int op = 0; op < array_size; op++){
		if(pthread_mutex_lock(&buffer) < 0){
			perror("[Error] Could not lock the mutex");
			exit(-1);
		}
		// Wait until the queue isn't full
		while (queue_full(q)){
			if(pthread_cond_wait(&can_put, &buffer) < 0){
				perror("[Error] Could not wait for condition \"can_put\"");
				exit(-1);
			}
		}
		if(queue_put(q, &prod_ops[op])< 0){
			perror("[Error] The queue is full");
			exit(-1);
		}
		if(pthread_cond_signal(&can_get)< 0){
			perror("[Error] Could not signal the condition \"can_get\"");
			exit(-1);
		}
		if(pthread_mutex_unlock(&buffer) < 0){
			perror("[Error] Could not unlock the mutex");
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
			perror("[Error] Could not lock the mutex");
			exit(-1);
		}
		// Wait until the queue isn't empty
		while (queue_empty(q)){
			if(pthread_cond_wait(&can_get, &buffer) < 0){
				perror("[Error] Could not wait for condition \"can_get\"");
				exit(-1);
			}
		}
		data = queue_get(q);
		if(data == NULL){
			perror("[Error] Queue is empty");
			exit(-1);
		}
		total += type_to_cost[data->type] * data->time;
		if(pthread_cond_signal(&can_put) < 0){
			perror("[Error] Could not signal the condition \"can_put\"");
			exit(-1);
		}
		if(pthread_mutex_unlock(&buffer) < 0){
			perror("[Error] Could not unlock the mutex");
			exit(-1);
		}
		remaining_ops--;
	}
	pthread_exit(0);

}
/*************************************/

int main (int argc, const char * argv[] ) {
	// Input error checking
	/*************************************/
	if (argc != 4){
		perror("[Error] Number of args invalid");
		exit(-1);
	}

	if (NUM_CONSUMERS < 1){
		perror("[Error] Number of consumers invalid");
		exit(-1);
	}

	int producer_num = atoi(argv[2]);
	if (producer_num <= 0){
		perror("[Error] Number of producers invalid");
		exit(-1);
	}

	int queue_size = atoi(argv[3]);
	if (queue_size <= 0){
		perror("[Error] Circular buffer size invalid");
		exit(-1);
	}
	/*************************************/

	// Main functionality
	/*************************************/
	q = queue_init(queue_size);
	
	OpArray op_array[producer_num];
	int op_num = file_parser(argv[1], producer_num, op_array);
	if (op_num < 1){
		perror("[Error] Not enough operations");
		exit(-1);
	}

	// Error checking for mutex creation
	if(pthread_mutex_init(&buffer, NULL) < 0){
		perror("[Error] Could not init the mutex");
		exit(-1);
	}
	if(pthread_cond_init(&can_put, NULL) < 0){
		perror("[Error] Could not init the condition \"can_put\"");
		exit(-1);
	}
	if(pthread_cond_init(&can_get, NULL) < 0){
		perror("[Error] Could not init the condition \"can_get\"");
		exit(-1);
	}

	// Thread arrays declaration
	pthread_t producers[producer_num];
	pthread_t consumers[NUM_CONSUMERS];

	int assigned_ops = op_num / NUM_CONSUMERS,
        remainder_ops = op_num % NUM_CONSUMERS;


	// Create consumer threads
	for (int consumer = 0; consumer < NUM_CONSUMERS; consumer++){
		int ops_to_consume = assigned_ops + (remainder_ops > 0 ? 1 : 0);
		if(pthread_create(&consumers[consumer], NULL, (void *)&consume, (void *)&ops_to_consume) < 0){
			perror("[Error] Could not create consumer thread");
			exit(-1);
		}
	}

	// Create producer threads
	for (int producer = 0; producer < producer_num; producer++){
		if(pthread_create(&producers[producer], NULL, (void *)&produce, (void *)&op_array[producer]) < 0){
			perror("[Error] Could not create producer thread");
			exit(-1);
		}
	}

	// Join (wait) producer threads
	for (int producer = 0; producer < producer_num; producer++){
		if(pthread_join(producers[producer], NULL) < 0){
			perror("[Error] Could not join the producer thread");
			exit(-1);
		}
	}

	// Join (wait) consumer threads
	for (int consumer = 0; consumer < NUM_CONSUMERS; consumer++){
		if(pthread_join(consumers[consumer], NULL) < 0){
			perror("[Error] Could not join the consumer thread");
			exit(-1);
		}
	}

	printf("Total: %i €.\n", total);
	/*************************************/

	// Cleanup
	/*************************************/
	queue_destroy(q);
	if(pthread_mutex_destroy(&buffer) < 0){
		perror("[Error] Could not destroy the mutex");
		exit(-1);
	}
	if(pthread_cond_destroy(&can_put) < 0){
		perror("[Error] Could not destroy the condition \"can_put\"");
		exit(-1);
	}
	if(pthread_cond_destroy(&can_get) < 0){
		perror("[Error] Could not destroy the condition \"can_get\"");
		exit(-1);
	}
	/*************************************/

    return 0;
}
