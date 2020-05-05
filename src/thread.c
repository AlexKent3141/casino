#include "thread.h"

int CreateMutex(CAS_Mutex* lock)
{
    return pthread_mutex_init(lock, NULL);
}

void DestroyMutex(CAS_Mutex* lock)
{
    pthread_mutex_destroy(lock);
}

void LockMutex(CAS_Mutex* lock)
{
    pthread_mutex_lock(lock);
}

void UnlockMutex(CAS_Mutex* lock)
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
