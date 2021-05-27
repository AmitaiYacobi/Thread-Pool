#include<stdio.h>
#include<string.h>
#include<pthread.h>
#include<stdlib.h>
#include<unistd.h>
#include <errno.h>

#include "threadPool.h"

/* ####################### Globals ############################### */

int destroyWasCalled = 0;
int stop = 0;

/* ####################### Protoypes ############################# */

void* doTask(void* arg);
void waitForThreads(ThreadPool* tp);

/* ####################### Implementations ####################### */

/*Insert new task to the queue*/
int tpInsertTask(ThreadPool* threadPool, void (*computeFunc) (void*), void* param) {
    int retcode;
    if (destroyWasCalled) {
        return -1;
    }
    Task* newTask = malloc(sizeof(Task));
    if (newTask == NULL) {
        perror("Error in system call");
        return -1;
    }
    newTask->function = computeFunc;
    newTask->param = param;
    osEnqueue(threadPool->tasksQueue, newTask);
}

/*Destroy the thread pool*/
void tpDestroy(ThreadPool* threadPool, int shouldWaitForTasks) {
    destroyWasCalled = 1;
    if (threadPool == NULL) return;
    if (shouldWaitForTasks) {
        waitForThreads(threadPool);
    }
    else if (!shouldWaitForTasks) {
        stop = 1;
        waitForThreads(threadPool);
    }
    pthread_mutex_destroy(&(threadPool->mutex));
    pthread_cond_destroy(&(threadPool->cond));
    osDestroyQueue(threadPool->tasksQueue);
    free(threadPool->threads);
    free(threadPool);
}

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
    pthread_mutex_init(&(tp->mutex), NULL);
    pthread_cond_init(&(tp->cond), NULL);

    /*Threads initialization*/
    for (i = 0; i < numOfThreads; i++) {
        retcode = pthread_create(threads[i], NULL, doTask, (void*)tp);
        if (retcode != 0) {
            perror("Thread creation failed with error");
        }
    }

    return tp;
}

/*Execute the tasks in the queue*/
void* doTask(void* arg) {
    int i, retcode;
    ThreadPool* tp = (ThreadPool*)arg;
    void (*func)(void*);
    void* arg;
    pthread_mutex_lock(&(tp->mutex));
    while (!stop) {
        if (!osIsQueueEmpty(tp->tasksQueue)) {
            Task* task = osDequeue(tp->tasksQueue);
            pthread_mutex_unlock(&(tp->mutex));
            func = task->function;
            arg = task->param;
            func(arg);
            free(task);
            pthread_mutex_lock(&(tp->mutex));
        }
        else {
            pthread_cond_wait(&(tp->cond), &(tp->mutex));
        }
    }
}

void waitForThreads(ThreadPool* tp) {
    int i;
    for (i = 0; i < tp->numOfThreads; i++)
        pthread_join(&(tp->threads[i]), NULL);
}