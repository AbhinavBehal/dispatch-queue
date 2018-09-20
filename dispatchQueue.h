/* 
 * File:   dispatchQueue.h
 * Author: robert
 *
 * Modified by: abeh957
 */

#ifndef DISPATCHQUEUE_H
#define	DISPATCHQUEUE_H

#include <pthread.h>
#include <semaphore.h>
#include <stdbool.h>

#define error_exit(MESSAGE)     perror(MESSAGE), exit(EXIT_FAILURE)

    typedef enum { // whether dispatching a task synchronously or asynchronously
        ASYNC, SYNC
    } task_dispatch_type_t;
    
    typedef enum { // The type of dispatch queue.
        CONCURRENT, SERIAL
    } queue_type_t;

    typedef struct task {
        char name[64];              // to identify it when debugging
        void (*work)(void *);       // the function to perform
        void *params;               // parameters to pass to the function
        task_dispatch_type_t type;  // asynchronous or synchronous
        sem_t sync_sem;             // semaphore to wait on if synchronous task
    } task_t;

    typedef struct dispatch_queue_item_t  dispatch_queue_item_t; // the dispatch queue item type
    typedef struct dispatch_queue_t dispatch_queue_t; // the dispatch queue type
    typedef struct dispatch_queue_thread_t dispatch_queue_thread_t; // the dispatch queue thread type

    struct dispatch_queue_thread_t {
        dispatch_queue_t *queue;    // the queue this thread is associated with
        pthread_t thread;           // the thread which runs the task
        sem_t thread_semaphore;     // the semaphore the thread waits on until a task is allocated
        task_t *task;               // the current task for this tread
    };

    struct dispatch_queue_item_t {
        task_t *task;                   // the task associated with this item
        dispatch_queue_item_t *next;    // pointer to the next item in the queue
    };

    struct dispatch_queue_t {
        queue_type_t queue_type;            // the type of queue - serial or concurrent
        pthread_t *thread_pool;             // the thread pool associated with the queue
        int pool_size;                      // the size of the thread pool
        pthread_mutex_t queue_mutex;        // mutex used to synchronise access to the queue
        pthread_cond_t queue_cond;          // condition variable used to signal events to the threads in the thread pool
        dispatch_queue_item_t *front;       // pointer to the first item in the queue
        dispatch_queue_item_t *back;        // pointer to the last item in the queue
        volatile bool shutdown;             // flag to indicate if the queue has been asked to shutdown (destroy)
        volatile bool waiting;              // flag to indicate if the queue is waiting for all currently queued tasks to finish
    };
    
    task_t *task_create(void (*)(void *), void *, char*);
    
    void task_destroy(task_t *);

    dispatch_queue_t *dispatch_queue_create(queue_type_t);
    
    void dispatch_queue_destroy(dispatch_queue_t *);
    
    int dispatch_async(dispatch_queue_t *, task_t *);
    
    int dispatch_sync(dispatch_queue_t *, task_t *);
    
    void dispatch_for(dispatch_queue_t *, long, void (*)(long));
    
    int dispatch_queue_wait(dispatch_queue_t *);

    void push_queue(dispatch_queue_t *, task_t *);

    dispatch_queue_item_t *pop_queue(dispatch_queue_t *);

    void queue_thread(void *);

#endif	/* DISPATCHQUEUE_H */