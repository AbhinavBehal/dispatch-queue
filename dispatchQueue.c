#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/sysinfo.h>

#include "dispatchQueue.h"

task_t *task_create(void (*work)(void *), void *param, char *name) {
    task_t *task = malloc(sizeof(task_t));
    if (!task)
        error_exit("Could not allocate memory for task\n");

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
    if (!queue)
        error_exit("Could not allocate memory for queue\n");

    queue->queue_type = queue_type;
    queue->front = NULL;
    queue->back = NULL;
    queue->shutdown = false;
    queue->waiting = false;

    if (pthread_mutex_init(&queue->queue_mutex, NULL))
        error_exit("Could not initialise queue mutex\n");
    if (pthread_cond_init(&queue->queue_cond, NULL))
        error_exit("Could not initialise queue condition variable\n");

    switch (queue->queue_type) {
    case CONCURRENT:
        queue->pool_size = get_nprocs();
        break;
    case SERIAL:
        queue->pool_size = 1;
        break;
    default:
        error_exit("Unknown queue_type\n");
    }
    queue->thread_pool = malloc(sizeof(pthread_t) * queue->pool_size);

    // create all the threads in the thread pool
    for (int i = 0; i < queue->pool_size; ++i) {
        if (pthread_create(&queue->thread_pool[i], NULL, (void *)queue_thread,
                           (void *)queue))
            error_exit("Could not create thread\n");
    }

    return queue;
}

void dispatch_queue_destroy(dispatch_queue_t *queue) {
    // signal the threads to shutdown
    queue->shutdown = true;
    pthread_mutex_lock(&queue->queue_mutex);
    pthread_cond_broadcast(&queue->queue_cond);
    pthread_mutex_unlock(&queue->queue_mutex);

    free(queue->thread_pool);

    dispatch_queue_item_t *item = NULL;
    // delete all remaining items still in the queue
    while ((item = pop_item(queue)) != NULL) {
        task_destroy(item->task);
        free(item);
    }
    pthread_mutex_destroy(&queue->queue_mutex);
    pthread_cond_destroy(&queue->queue_cond);
    free(queue);
}

int dispatch_async(dispatch_queue_t *queue, task_t *task) {
    pthread_mutex_lock(&queue->queue_mutex);
    // ignore the task if the queue is waiting or shutdown
    if (!queue->shutdown && !queue->waiting) {
        task->type = ASYNC;
        push_item(queue, task);
        pthread_cond_signal(&queue->queue_cond);
    }
    pthread_mutex_unlock(&queue->queue_mutex);
    return 0;
}

int dispatch_sync(dispatch_queue_t *queue, task_t *task) {
    pthread_mutex_lock(&queue->queue_mutex);
    // ignore the task if the queue is waiting or shutdown
    if (!queue->shutdown && !queue->waiting) {
        task->type = SYNC;
        push_item(queue, task);
        pthread_cond_signal(&queue->queue_cond);
        pthread_mutex_unlock(&queue->queue_mutex);
        // wait for the task to be completed
        sem_wait(&task->sync_sem);
        task_destroy(task);
    } else {
        pthread_mutex_unlock(&queue->queue_mutex);
    }
    return 0;
}

void dispatch_for(dispatch_queue_t *queue, long number, void (*work)(long)) {
    for (long i = 0; i < number; ++i) {
        task_t *task = task_create((void (*)(void *))work, (void *)i, "");
        dispatch_async(queue, task);
    }
    dispatch_queue_wait(queue);
    dispatch_queue_destroy(queue);
}

int dispatch_queue_wait(dispatch_queue_t *queue) {
    // signal the threads to exit once the queue is empty
    pthread_mutex_lock(&queue->queue_mutex);
    queue->waiting = true;
    pthread_cond_broadcast(&queue->queue_cond);
    pthread_mutex_unlock(&queue->queue_mutex);

    // wait for all threads to finish
    for (int i = 0; i < queue->pool_size; ++i) {
        pthread_join(queue->thread_pool[i], NULL);
    }
    return 0;
}

// Function that is executed by each thread in the thread pool
void queue_thread(void *dispatch_queue) {
    dispatch_queue_t *queue = (dispatch_queue_t *)dispatch_queue;
    while (!queue->shutdown) {
        pthread_mutex_lock(&queue->queue_mutex);
        // wait for an item to be added to the queue if it's currently empty
        // need a while loop here in case of spurious wake-ups from cond_wait
        while (!queue->shutdown && !queue->waiting && queue->front == NULL) {
            pthread_cond_wait(&queue->queue_cond, &queue->queue_mutex);
        }
        // if the queue is shutdown, or is waiting and the queue is empty, the
        // thread should stop running
        if (queue->shutdown || (queue->waiting && queue->front == NULL)) {
            pthread_mutex_unlock(&queue->queue_mutex);
            break;
        }
        dispatch_queue_item_t *item = pop_item(queue);
        pthread_mutex_unlock(&queue->queue_mutex);

        item->task->work(item->task->params);
        if (item->task->type == SYNC) {
            // don't destroy the task if synchronous
            // let the dispatch_sync function do this as we want the semaphore
            // to stay alive
            sem_post(&item->task->sync_sem);
        } else {
            task_destroy(item->task);
        }
        free(item);
    }
}

// Helper function to push tasks onto a queue
void push_item(dispatch_queue_t *queue, task_t *task) {
    dispatch_queue_item_t *item = malloc(sizeof(dispatch_queue_item_t));
    item->task = task;
    item->next = NULL;

    if (queue->back == NULL) {
        queue->front = item;
        queue->back = item;
    } else {
        queue->back->next = item;
        queue->back = item;
    }
}

// Helper function to extract the task currently at the front of a queue
dispatch_queue_item_t *pop_item(dispatch_queue_t *queue) {
    if (queue->front == NULL) {
        return NULL;
    } else {
        dispatch_queue_item_t *item = queue->front;
        queue->front = queue->front->next;
        if (queue->front == NULL) {
            queue->back = NULL;
        }
        return item;
    }
}
