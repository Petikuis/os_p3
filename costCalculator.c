
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
#define BUFFER_SIZE 64

enum{COMMON, COMPUTATION, SUPER};

typedef struct operation{
	int id;
	Item *item;
}Operation;

int type_to_cost[] = {
	[COMMON] = 1,
	[COMPUTATION] = 3,
	[SUPER] = 10
};

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

Operation* op_init(int id, int type, int time){

    Operation *op = (Operation *)malloc(sizeof(Operation));
    op->id = id;
    op->item = item_init(type, time);

    return op;
}

int file_parser(const char *filename, int producer_num, Operation **op_array){
	int op_num;
	apply_input_redirection(filename);
	scanf("%d", &op_num);
	int assigned_ops = op_num / producer_num, 
		remainder_ops = op_num % producer_num;

	for(int producer = 0; producer < producer_num; producer++){
    	int ops_to_insert = assigned_ops + (remainder_ops > 0 ? 1 : 0);
    	remainder_ops--;
    	op_array[producer] = (Operation *)malloc(ops_to_insert * sizeof(Operation));
    	for(int operation = 0; operation < ops_to_insert; operation++){
    		int id, type, time;
        	scanf("%d %d %d", &id, &type, &time);
        	op_array[producer][operation] = *op_init(id, --type, time);
    	}
	}
	return op_num;

}

int get_op_array_size(int op_num, int producer_num, int total_producers){
    int base_ops = op_num / total_producers,
        remainder_ops = op_num % total_producers;
    return base_ops + (producer_num < remainder_ops ? 1 : 0);
}


/**
 * Entry point
 * @param argc
 * @param argv
 * @return
 */
int main (int argc, const char * argv[] ) {

	const char *filename = argv[1];
	int producer_num = atoi(argv[2]);
	Operation *op_array[producer_num];


	int op_num = file_parser(filename, producer_num, op_array);

	for (int producer = 0; producer < producer_num; producer++){
		int array_size = get_op_array_size(op_num, producer, producer_num);
		for (int op = 0; op < array_size; op++){
			printf("Operation assigned to producer %d with id %d, type %d and time %d\n", 
					producer, op_array[producer][op].id, 
					(op_array[producer][op].item)->type, 
					(op_array[producer][op].item)->time);
		}

	}



	/*Queue *q = queue_init(5);
	queue_put(q, item_init(COMMON, 3));

	queue_put(q, item_init(COMPUTATION, 5));

	queue_put(q, item_init(SUPER, 7));

	queue_put(q, item_init(COMMON, 7));

	queue_put(q, item_init(COMPUTATION, 7));

	int i = 0;

	while (!queue_empty(q)){
		Item *my_item = queue_get(q);
		printf("The %d item has type %d and the time is %d\n", i, my_item->type, my_item->time);
		if (i == 0) queue_put(q, item_init(SUPER, 10));
		i++;
	}

	queue_destroy(q);


    int total = 0;
    printf("Total: %i €.\n", total);*/

    return 0;
}
