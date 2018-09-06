/* 
 * File:   test3.c
 * Author: robert
 */

#include "dispatchQueue.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

void test3() {
    sleep(1);
    printf("test3 running\n");
}

/*
 * Checks waiting on a queue.
 * The output from test3 should appear before the program finishes.
 * 
 */
int main(int argc, char** argv) {
    // create a concurrent dispatch queue
    dispatch_queue_t * concurrent_dispatch_queue;
    task_t *task;
    concurrent_dispatch_queue = dispatch_queue_create(CONCURRENT);
    task = task_create(test3, NULL, "test3");
    dispatch_async(concurrent_dispatch_queue, task);
    printf("Safely dispatched\n");
    dispatch_queue_wait(concurrent_dispatch_queue);
    dispatch_queue_destroy(concurrent_dispatch_queue);
    return EXIT_SUCCESS;
}