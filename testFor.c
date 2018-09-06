/* 
 * File:   testFor.c
 * Author: robert
 */

#include "dispatchQueue.h"
#include <stdio.h>
#include <stdlib.h>

void for_test(long i) {
    long counter = 0;
    for (; i < 1000000000; i++)
        counter++;
    printf("The number is %ld\n", counter);
}

// Only required for SE 370 students.
int main(int argc, char** argv) {
    dispatch_queue_t *queue;
    queue = dispatch_queue_create(CONCURRENT);
    dispatch_for(queue, 10, for_test);
    return (EXIT_SUCCESS);
}

