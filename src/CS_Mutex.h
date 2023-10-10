#pragma once

#include "common.h"

typedef struct CS_Mutex
{
#ifdef C11_THREADS
	mtx_t mutex;
#endif
#ifdef WIN32_THREADS
	HANDLE mutex;
#endif
#ifdef POSIX_THREADS
	pthread_mutex_t mutex;
#endif
} CS_Mutex;

char csm_create(CS_Mutex* mutex);
char csm_destroy(CS_Mutex* mutex);

char csm_lock(CS_Mutex* mutex);
char csm_try_lock(CS_Mutex* mutex);
char csm_unlock(CS_Mutex* mutex);

inline char csm_create(CS_Mutex* mutex)
{
#ifdef C11_THREADS
	return mtx_init(&mutex->mutex, mtx_plain) != thrd_success;
#endif
#ifdef WIN32_THREADS
	mutex->mutex = CreateMutexA(0, 0, 0);
	return !mutex->mutex;
#endif
#ifdef POSIX_THREADS
	return pthread_mutex_init(&mutex->mutex, 0);
#endif
}

inline char csm_destroy(CS_Mutex* mutex)
{
#ifdef C11_THREADS
	mtx_destroy(&mutex->mutex);
	memset(mutex, 0, sizeof(CS_Mutex));
	return 0;
#endif
#ifdef WIN32_THREADS
	BOOL res = CloseHandle(mutex->mutex);
	memset(mutex, 0, sizeof(CS_Mutex));
	return !res;
#endif
#ifdef POSIX_THREADS
	int res = pthread_mutex_destroy(&mutex->mutex);
	memset(mutex, 0, sizeof(CS_Mutex));
	return res;
#endif
}

inline char csm_lock(CS_Mutex* mutex)
{
#ifdef C11_THREADS
	return mtx_lock(&mutex->mutex) != thrd_success;
#endif
#ifdef WIN32_THREADS
	return WaitForSingleObject(mutex->mutex, INFINITE);
#endif
#ifdef POSIX_THREADS
	return pthread_mutex_lock(&mutex->mutex);
#endif
}

inline char csm_try_lock(CS_Mutex* mutex)
{
#ifdef C11_THREADS
	return mtx_trylock(&mutex->mutex) != thrd_success;
#endif
#ifdef WIN32_THREADS
	return WaitForSingleObject(mutex->mutex, 0);
#endif
#ifdef POSIX_THREADS
	return pthread_mutex_trylock(&mutex->mutex);
#endif
}

char csm_unlock(CS_Mutex* mutex)
{
#ifdef C11_THREADS
	return mtx_unlock(&mutex->mutex) != thrd_success;
#endif
#ifdef WIN32_THREADS
	return !ReleaseMutex(mutex->mutex);
#endif
#ifdef POSIX_THREADS
	return pthread_mutex_unlock(&mutex->mutex);
#endif
}