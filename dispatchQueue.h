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

    typedef struct dispatch_queue_node_t  dispatch_queue_node_t;
    typedef struct dispatch_queue_t dispatch_queue_t; // the dispatch queue type
    typedef struct dispatch_queue_thread_t dispatch_queue_thread_t; // the dispatch queue thread type

    // TODO: REMOVE REDUNDANT STUFF FROM THIS (should I be using this?)
    struct dispatch_queue_thread_t {
        dispatch_queue_t *queue;// the queue this thread is associated with
        pthread_t thread;       // the thread which runs the task
        //sem_t* thread_semaphore; // the semaphore the thread waits on until a task is allocated
        //task_t *task;           // the current task for this tread
    };

    struct dispatch_queue_node_t {
        task_t *task;
        dispatch_queue_node_t *next;
    };

    struct dispatch_queue_t {
        queue_type_t queue_type;            // the type of queue - serial or concurrent
        /*sem_t has_item;                     // semaphore the dispatcher waits on until a task is in the queue
        sem_t excl_sem;                     // semaphore to wait on to get exclusive access to the queue*/
        pthread_t *thread_pool;             // the thread pool associated with the queue
        int pool_size;                      // the size of the thread pool
        pthread_mutex_t queue_mutex;
        pthread_cond_t queue_cond;
        //pthread_t dispatch_thread;
        dispatch_queue_node_t *front;       // pointer to the first node in the queue
        dispatch_queue_node_t *back;        // pointer to the last node in the queue
        bool shutdown;                      // flag to determine if new tasks are allowed to get added to the queue
        bool waiting;
    };
    
    task_t *task_create(void (*)(void *), void *, char*);
    
    void task_destroy(task_t *);

    dispatch_queue_t *dispatch_queue_create(queue_type_t);
    
    void dispatch_queue_destroy(dispatch_queue_t *);
    
    void dispatch_async(dispatch_queue_t *, task_t *);
    
    void dispatch_sync(dispatch_queue_t *, task_t *);
    
    void dispatch_for(dispatch_queue_t *, long, void (*)(long));
    
    void dispatch_queue_wait(dispatch_queue_t *);

    void push(dispatch_queue_t *, task_t *);

    dispatch_queue_node_t *pop(dispatch_queue_t *);

    void queue_thread(void *);

    void dispatch_thread(void *);

#endif	/* DISPATCHQUEUE_H */