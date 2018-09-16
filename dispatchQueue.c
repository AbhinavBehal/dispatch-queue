#include <stdlib.h>
#include <string.h>
#include <sys/sysinfo.h>
#include <stdio.h>

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

    switch (queue_type) {
    case CONCURRENT:
        queue->pool_size = get_nprocs();
        break;
    case SERIAL:
        queue->pool_size = 1;
        break;
    }
    queue->thread_pool = (pthread_t *)malloc(sizeof(pthread_t) * queue->pool_size);
    // TODO: Should this be using dispatch_queue_thread_t?
    for (int i = 0; i < queue->pool_size; ++i) {
        pthread_create(&queue->thread_pool[i], NULL, (void *)queue_thread, (void *)queue);
    }

    return queue;
}

void dispatch_queue_destroy(dispatch_queue_t *queue) {
    // TODO:
    // use pthread_cancel here?
}

int dispatch_async(dispatch_queue_t *queue, task_t *task) {
    // TODO: check if queue is in waiting state
    sem_wait(&queue->excl_sem);
    printf("task pushed\n");
    push(queue, task);
    sem_post(&queue->sem);
    sem_post(&queue->excl_sem);
}

int dispatch_sync(dispatch_queue_t *queue, task_t *task) {

}

void dispatch_for(dispatch_queue_t *queue, long number, void (*work)(long)) {

}

int dispatch_queue_wait(dispatch_queue_t *queue) {
    // somehow signal the threads to gracefully stop
    // pthread_cancel doesn't work
    // HAVE TO MAKE EACH THREAD HAVE ITS OWN SEMAPHORE
    // INCREMENT IT & HAVE THE THREAD CHECK TO SEE IF IT SHOULD STOP
    /*for (int i = 0; i < queue->pool_size; ++i) {
        pthread_cancel(queue->thread_pool[i]);
    }*/
    for (int i = 0; i < queue->pool_size; ++i) {
        pthread_join(queue->thread_pool[i], NULL);
    }
    return 0;
}

void queue_thread(void *dispatch_queue) {
    printf("thread started\n");
    dispatch_queue_t *queue = (dispatch_queue_t *)dispatch_queue;
    for(;;) { // TODO: handle thread cleanup when destroying or waiting for queue (maybe signal threads to stop)
        dispatch_queue_node_t *node = NULL;
        if (sem_wait(&queue->sem) == -1) {
            printf("SIGNALLED TO STOP\n");
            break;
        }
        if (sem_wait(&queue->excl_sem) == -1) {
            printf("SIGNALLED TO STOP EXCL\n");
            break;
        }
        node = pop(queue);
        sem_post(&queue->excl_sem);
        node->task->work(node->task->params);
        task_destroy(node->task);
        free(node);
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
        node->next = NULL;
        queue->front = queue->front->next;
        if (queue->front == NULL) {
            queue->back = NULL;
        }
        return node;
    }
}
