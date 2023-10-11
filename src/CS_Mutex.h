#pragma once

#include "common.h"

typedef struct CS_Mutex
{
#ifdef C11_THREADS
	mtx_t mutex_handle;
#endif
#ifdef WIN32_THREADS
	HANDLE mutex_handle;
#endif
#ifdef POSIX_THREADS
	pthread_mutex_t mutex_handle;
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
	return mtx_init(&mutex->mutex_handle, mtx_plain) != thrd_success;
#endif
#ifdef WIN32_THREADS
	mutex->mutex_handle = CreateMutexA(0, 0, 0);
	return !mutex->mutex_handle;
#endif
#ifdef POSIX_THREADS
	return pthread_mutex_init(&mutex->mutex_handle, 0);
#endif
}

inline char csm_destroy(CS_Mutex* mutex)
{
#ifdef C11_THREADS
	mtx_destroy(&mutex->mutex_handle);
	memset(mutex, 0, sizeof(CS_Mutex));
	return 0;
#endif
#ifdef WIN32_THREADS
	BOOL res = CloseHandle(mutex->mutex_handle);
	memset(mutex, 0, sizeof(CS_Mutex));
	return !res;
#endif
#ifdef POSIX_THREADS
	int res = pthread_mutex_destroy(&mutex->mutex_handle);
	memset(mutex, 0, sizeof(CS_Mutex));
	return res;
#endif
}

inline char csm_lock(CS_Mutex* mutex)
{
#ifdef C11_THREADS
	return mtx_lock(&mutex->mutex_handle) != thrd_success;
#endif
#ifdef WIN32_THREADS
	return WaitForSingleObject(mutex->mutex_handle, INFINITE);
#endif
#ifdef POSIX_THREADS
	return pthread_mutex_lock(&mutex->mutex_handle);
#endif
}

inline char csm_try_lock(CS_Mutex* mutex)
{
#ifdef C11_THREADS
	return mtx_trylock(&mutex->mutex_handle) != thrd_success;
#endif
#ifdef WIN32_THREADS
	return WaitForSingleObject(mutex->mutex_handle, 0);
#endif
#ifdef POSIX_THREADS
	return pthread_mutex_trylock(&mutex->mutex_handle);
#endif
}

char csm_unlock(CS_Mutex* mutex)
{
#ifdef C11_THREADS
	return mtx_unlock(&mutex->mutex_handle) != thrd_success;
#endif
#ifdef WIN32_THREADS
	return !ReleaseMutex(mutex->mutex_handle);
#endif
#ifdef POSIX_THREADS
	return pthread_mutex_unlock(&mutex->mutex_handle);
#endif
}