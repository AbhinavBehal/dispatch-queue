/* 
 * File:   test4.c
 * Author: robert
 */

#include "dispatchQueue.h"
#include <stdio.h>
#include <stdlib.h>

volatile long counter = 0;

void increment(void *identifier) {
    printf("task \"%s\"\n", (char *) identifier);
    long i;
    for (i = 0; i < 1000000000; i++)
        counter++;
}

/*
 * Checks to see if tasks are concurrent.
 */
int main(int argc, char** argv) {
    // create a concurrent dispatch queue
    dispatch_queue_t * concurrent_dispatch_queue;
    task_t *task;
    concurrent_dispatch_queue = dispatch_queue_create(CONCURRENT);
    char id;
    char names[10][2];  // because these are going to be parameters to tasks they have to hang around
    for (id = 'A'; id <= 'J'; id++) {
        char *name = names[id - 'A'];
        name[0] = id; name[1] = '\0';
        task = task_create(increment, (void *)name, name);
        dispatch_async(concurrent_dispatch_queue, task);
    }
    printf("Safely dispatched\n");
    dispatch_queue_wait(concurrent_dispatch_queue);
    printf("The counter is %ld\n", counter);
    dispatch_queue_destroy(concurrent_dispatch_queue);
    return EXIT_SUCCESS;
}