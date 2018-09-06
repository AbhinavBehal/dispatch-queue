/* 
 * File:   test1.c
 * Author: robert
 */

#include "dispatchQueue.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

void test1() {
    sleep(1);
    printf("test1 running\n");
}

/*
 * Checks that a CONCURRENT dispatch queue and a task can be constructed.
 * Then synchronously dispatches the task to the queue.
 */
int main(int argc, char** argv) {
    // create a concurrent dispatch queue
    dispatch_queue_t *concurrent_dispatch_queue;
    task_t *task;
    concurrent_dispatch_queue = dispatch_queue_create(CONCURRENT);
    task = task_create(test1, NULL, "test1");
    dispatch_sync(concurrent_dispatch_queue, task);
    printf("Safely dispatched\n");
    dispatch_queue_destroy(concurrent_dispatch_queue);
    return EXIT_SUCCESS;
}