#include <stdlib.h>
#include <string.h>

#include "dispatchQueue.h"

task_t *task_create(void (* work)(void *), void *param, char* name) {
    task_t *new_task = malloc(sizeof(task_t));
    strcpy(new_task->name, name);
    new_task->work = work;
    new_task->params = param;
    // ASYNC by default
    new_task->type = ASYNC;
}
    
void task_destroy(task_t *task) {
    free(task);
}

dispatch_queue_t *dispatch_queue_create(queue_type_t queue_type) {
    dispatch_queue_t *new_queue = malloc(sizeof(dispatch_queue_t));
    new_queue->type = queue_type;
    new_queue->front = NULL;
    new_queue->back = NULL;
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

dispatch_queue_node_t *create_node(task_t *task) {
    dispatch_queue_node_t *node = malloc(sizeof(dispatch_queue_node_t));
    node->task = task;
    node->next = NULL;
    return node;
}

void push(dispatch_queue_t *queue, task_t *task) {
    dispatch_queue_node_t *node = create_node(task);
    if (queue->rear == NULL) {
        queue->front = node;
        queue->back = node;
    } else {
        queue->back->next = node;
        queue->back = node;
    }
}

dispatch_queue_node_t pop(dispatch_queue_t *queue) {
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
