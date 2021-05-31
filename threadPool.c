
#include <pthread.h>
#include <stdio.h>

#include "threadPool.h"


/* ####################### Protoypes ############################# */

void* doTask(void* arg);
void waitForThreads(ThreadPool* tp);

/* ####################### Implementations ####################### */

/*Create thread pool*/
ThreadPool* tpCreate(int numOfThreads) {
    int i, retcode;
    ThreadPool* tp = malloc(sizeof(ThreadPool));
    if (tp == NULL) {
        perror("Error in system call");
        free(tp);
        return NULL;
    }
    pthread_t* threads = malloc(sizeof(pthread_t) * numOfThreads);
    if (threads == NULL) {
        perror("Error in system call");
        free(threads);
        return NULL;
    }
    OSQueue* tasksQueue = osCreateQueue();
    if (tasksQueue == NULL) {
        perror("Error in system call");
        osDestroyQueue(tasksQueue);
        return NULL;
    }

    tp->tasksQueue = tasksQueue;
    tp->threads = threads;
    tp->numOfThreads = numOfThreads;
    tp->destroyWasCalled = 0;
    tp->stop = 0;
    pthread_mutex_init(&(tp->mutex), NULL);
    pthread_cond_init(&(tp->cond), NULL);

    /*Threads initialization*/
    for (i = 0; i < numOfThreads; i++) {
        retcode = pthread_create(&threads[i], NULL, doTask, (void*)tp);
        if (retcode != 0) {
            perror("Thread creation failed with error");
        }
    }
    return tp;
}

/*Insert new task to the queue*/
int tpInsertTask(ThreadPool* threadPool, void (*computeFunc) (void*), void* param) {
    int retcode;
    if (threadPool->destroyWasCalled) {
        return -1;
    }
    Task* newTask = malloc(sizeof(Task));
    if (newTask == NULL) {
        perror("Error in system call");
        free(newTask);
        return -1;
    }
    newTask->function = computeFunc;
    newTask->param = param;
    osEnqueue(threadPool->tasksQueue, newTask);
    pthread_cond_signal(&(threadPool->cond));
    return 0;
}

/*Destroy the thread pool*/
void tpDestroy(ThreadPool* threadPool, int shouldWaitForTasks) {
    threadPool->destroyWasCalled = 1;
    if (threadPool == NULL) return;
    if (shouldWaitForTasks) {
        while (!osIsQueueEmpty(threadPool->tasksQueue)) {}
        threadPool->stop = 1;
        waitForThreads(threadPool);

    }
    else if (!shouldWaitForTasks) {
        threadPool->stop = 1;
        waitForThreads(threadPool);
    }
    /*Free all tasks in the queue*/
    while (!osIsQueueEmpty(threadPool->tasksQueue)) {
        Task* task = osDequeue(threadPool->tasksQueue);
        free(task);
    }
    osDestroyQueue(threadPool->tasksQueue);
    free(threadPool->threads);
    free(threadPool);
    pthread_mutex_destroy(&(threadPool->mutex));

}

/*Execute the tasks in the queue*/
void* doTask(void* arg) {
    int i, retcode;
    ThreadPool* tp = (ThreadPool*)arg;
    void (*func)(void*);
    void* param;
    pthread_mutex_lock(&(tp->mutex));
    while (!(tp->stop)) {
        if (!osIsQueueEmpty(tp->tasksQueue)) {
            Task* task = osDequeue(tp->tasksQueue);
            pthread_mutex_unlock(&(tp->mutex));
            func = task->function;
            param = task->param;
            func(param);
            free(task);
            pthread_mutex_lock(&(tp->mutex));
        }
        else {
            pthread_cond_wait(&(tp->cond), &(tp->mutex));
        }
    }
    pthread_mutex_unlock(&(tp->mutex));
}

void waitForThreads(ThreadPool* tp) {
    int i;
    pthread_cond_broadcast(&(tp->cond));
    for (i = 0; i < tp->numOfThreads; i++)
        pthread_join(tp->threads[i], NULL);
}