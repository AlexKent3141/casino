#ifndef __CASINO_THREAD_INCLUDED_H__
#define __CASINO_THREAD_INCLUDED_H__

#include "../include/casino.h"
#include "pthread.h"

typedef pthread_mutex_t mutex_t;
typedef pthread_t thread_t;

int CreateMutex(mutex_t*);
void DestroyMutex(mutex_t*);

void LockMutex(mutex_t*);
void UnlockMutex(mutex_t*);

int CreateThread(pthread_t*, void* (*)(void*), void*);
void JoinThread(pthread_t*);

#endif /* __CASINO_THREAD_INCLUDED_H__ */
