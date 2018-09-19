#include <stdlib.h>
#include <string.h>
#include <sys/sysinfo.h>
#include <stdio.h>

#include "dispatchQueue.h"

task_t *task_create(void (* work)(void *), void *param, char* name) {
    task_t *task = malloc(sizeof(task_t));
    task->work = work;
    task->params = param;
    strcpy(task->name, name);
    sem_init(&task->sync_sem, 0, 0);
    return task;
}
    
void task_destroy(task_t *task) {
    sem_destroy(&task->sync_sem);
    free(task);
}

dispatch_queue_t *dispatch_queue_create(queue_type_t queue_type) {
    dispatch_queue_t *queue = malloc(sizeof(dispatch_queue_t));
    queue->queue_type = queue_type;
    queue->front = NULL;
    queue->back = NULL;
    queue->shutdown = false;
    queue->waiting = false;
    pthread_mutex_init(&queue->queue_mutex, NULL);
    pthread_cond_init(&queue->queue_cond, NULL);

    switch (queue->queue_type) {
    case CONCURRENT:
        queue->pool_size = get_nprocs();
        break;
    case SERIAL:
        queue->pool_size = 1;
        break;
    default:
        error_exit("Unknown queue_type");
    }
    queue->thread_pool = malloc(sizeof(pthread_t) * queue->pool_size);

    for (int i = 0; i < queue->pool_size; ++i) {
        pthread_create(&queue->thread_pool[i], NULL, (void *)queue_thread, (void *)queue);
    }

    return queue;
}

void dispatch_queue_destroy(dispatch_queue_t *queue) {
    pthread_mutex_lock(&queue->queue_mutex);
    queue->shutdown = true;
    pthread_cond_broadcast(&queue->queue_cond);
    pthread_mutex_unlock(&queue->queue_mutex);

    for (int i = 0; i < queue->pool_size; ++i) {
        // TODO check if cancel is the best way
        pthread_cancel(queue->thread_pool[i]);
    }
    for (int i = 0; i < queue->pool_size; ++i) {
        pthread_join(queue->thread_pool[i], NULL);
    }
    free(queue->thread_pool);

    dispatch_queue_node_t *node = NULL;
    while ((node = pop(queue)) != NULL) {
        task_destroy(node->task);
        free(node);
    }
    pthread_mutex_destroy(&queue->queue_mutex);
    pthread_cond_destroy(&queue->queue_cond);
    free(queue);
}

void dispatch_async(dispatch_queue_t *queue, task_t *task) {
    // TODO check if task should be destroyed if added after shutdown or wait
    pthread_mutex_lock(&queue->queue_mutex);
    if (!queue->shutdown && !queue->waiting) {
        task->type = ASYNC;
        push(queue, task);
        pthread_cond_signal(&queue->queue_cond);
    }
    pthread_mutex_unlock(&queue->queue_mutex);
}

void dispatch_sync(dispatch_queue_t *queue, task_t *task) {
    // TODO check if task should be destroyed if added after shutdown or wait
    pthread_mutex_lock(&queue->queue_mutex);
    if (!queue->shutdown && !queue->waiting) {
        task->type = SYNC;
        push(queue, task);
        pthread_cond_signal(&queue->queue_cond);
        pthread_mutex_unlock(&queue->queue_mutex);

        sem_wait(&task->sync_sem);
        task_destroy(task);
    } else {
        pthread_mutex_unlock(&queue->queue_mutex);
    }
}

void dispatch_for(dispatch_queue_t *queue, long number, void (*work)(long)) {
    for (long i = 0; i < number; ++i) {
        task_t *task = task_create((void (*)(void *))work, (void *)i, "");
        dispatch_async(queue, task);
    }
    dispatch_queue_wait(queue);
    // TODO: check if this should cleanup the queue
    dispatch_queue_destroy(queue);
}

void dispatch_queue_wait(dispatch_queue_t *queue) {
    pthread_mutex_lock(&queue->queue_mutex);
    queue->waiting = true;
    pthread_cond_broadcast(&queue->queue_cond);
    pthread_mutex_unlock(&queue->queue_mutex);

    for (int i = 0; i < queue->pool_size; ++i) {
        pthread_join(queue->thread_pool[i], NULL);
    }
}

void queue_thread(void *dispatch_queue) {
    dispatch_queue_t *queue = (dispatch_queue_t *) dispatch_queue;
    while (!queue->shutdown) {
        pthread_mutex_lock(&queue->queue_mutex);
        while (!queue->shutdown && !queue->waiting && queue->front == NULL) {
            pthread_cond_wait(&queue->queue_cond, &queue->queue_mutex);
        }
        if (queue->shutdown || (queue->waiting && queue->front == NULL)) {
            pthread_mutex_unlock(&queue->queue_mutex);
            break;
        }
        dispatch_queue_node_t *node = pop(queue);
        pthread_mutex_unlock(&queue->queue_mutex);
        
        node->task->work(node->task->params);
        if (node->task->type == SYNC) {
            // don't destroy task if synchronous
            // let the dispatch_sycn function do this as we want the semaphore to stay alive
            sem_post(&node->task->sync_sem);
        } else {
            task_destroy(node->task);
        }
        free(node);
    }
}

void push(dispatch_queue_t *queue, task_t *task) {
    dispatch_queue_node_t *node = malloc(sizeof(dispatch_queue_node_t));
    node->task = task;
    node->next = NULL;

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
