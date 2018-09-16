#include <stdlib.h>
#include <string.h>

#include "dispatchQueue.h"

task_t *task_create(void (* work)(void *), void *param, char* name) {
    task_t *task = (task_t *)malloc(sizeof(task_t));
    task->work = work;
    task->params = param;
    strcpy(task->name, name);
    return task;
}
    
void task_destroy(task_t *task) {
    free(task);
}

dispatch_queue_t *dispatch_queue_create(queue_type_t queue_type) {
    dispatch_queue_t *queue = malloc(sizeof(dispatch_queue_t));
    queue->queue_type = queue_type;
    queue->front = NULL;
    queue->back = NULL;
    sem_init(&queue->excl_sem, 0, 1);
    sem_init(&queue->sem, 0, 0);
    // TODO: start dispatcher thread here
    return queue;
}

void dispatch_queue_destroy(dispatch_queue_t *queue) {

}

int dispatch_async(dispatch_queue_t *queue, task_t *task) {

}

int dispatch_sync(dispatch_queue_t *queue, task_t *task) {

}

void dispatch_for(dispatch_queue_t *queue, long number, void (*work)(long)) {

}

int dispatch_queue_wait(dispatch_queue_t *queue) {

}

void dispatch_thread(void *current_queue) {
    dispatch_queue_t* queue = (dispatch_queue_t *) current_queue;

    for (;;) {
        sem_wait(&queue->sem);
        sem_wait(&queue->excl_sem);
        // get task from queue
        dispatch_queue_node_t *node = pop(queue);
        // submit task to thread pool
        // clean up node?
        // free task?
        sem_post(&queue->excl_sem);
    }
}

dispatch_queue_node_t *create_node(task_t *task) {
    dispatch_queue_node_t *node = malloc(sizeof(dispatch_queue_node_t));
    node->task = task;
    node->next = NULL;
    return node;
}

void push(dispatch_queue_t *queue, task_t *task) {
    dispatch_queue_node_t *node = create_node(task);
    if (queue->back == NULL) {
        queue->front = node;
        queue->back = node;
    } else {
        queue->back->next = node;
        queue->back = node;
    }
}

dispatch_queue_node_t *pop(dispatch_queue_t *queue) {
    if (queue->front == NULL) {
        return NULL;
    } else {
        dispatch_queue_node_t *node = queue->front;
        queue->front = queue->front->next;
        if (queue->front == NULL) {
            queue->back = NULL;
        }
        return node;
    }
}
