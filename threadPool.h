#ifndef __THREAD_POOL__
#define __THREAD_POOL__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "osqueue.h"

typedef struct task {
    void* function;
    void* param;
} Task;

typedef struct thread_pool {
    OSQueue* tasksQueue;
    pthread_t* threads;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    int numOfThreads;
    int destroyWasCalled;
    int stop;
} ThreadPool;

ThreadPool* tpCreate(int numOfThreads);

void tpDestroy(ThreadPool* threadPool, int shouldWaitForTasks);

int tpInsertTask(ThreadPool* threadPool, void (*computeFunc)(void*), void* param);

#endif
