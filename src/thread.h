#ifndef __CASINO_THREAD_INCLUDED_H__
#define __CASINO_THREAD_INCLUDED_H__

#include "../include/casino.h"
#include "pthread.h"

typedef pthread_t thread_t;

int CreateMutex(CAS_Mutex*);
void DestroyMutex(CAS_Mutex*);

void LockMutex(CAS_Mutex*);
void UnlockMutex(CAS_Mutex*);

int CreateThread(pthread_t*, void* (*)(void*), void*);
void JoinThread(pthread_t*);

#endif /* __CASINO_THREAD_INCLUDED_H__ */
