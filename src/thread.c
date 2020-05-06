#include "thread.h"

int CreateMutex(mutex_t* lock)
{
    return pthread_mutex_init(lock, NULL);
}

void DestroyMutex(mutex_t* lock)
{
    pthread_mutex_destroy(lock);
}

void LockMutex(mutex_t* lock)
{
    pthread_mutex_lock(lock);
}

void UnlockMutex(mutex_t* lock)
{
    pthread_mutex_unlock(lock);
}

int CreateThread(
    thread_t* tid,
    void* (*func)(void*),
    void* arg)
{
    return pthread_create(tid, NULL, func, arg);
}

void JoinThread(thread_t* tid)
{
    pthread_join(*tid, NULL);
}

int CancelThread(
    thread_t tid)
{
    return pthread_cancel(tid);
}

void CheckForCancel()
{
    pthread_testcancel();
}
