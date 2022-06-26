#pragma once

#if __STDC_VERSION__ >= 201112L && !defined(__STDC_NO_THREADS__)
#define C11_THREADS
#include <threads.h>
#endif

#if defined(_WIN32) && !defined(C11_THREADS)
#define WIN32_THREADS
#include <windows.h>
#endif

#if defined(__unix__) && !defined(C11_THREADS)
#define POSIX_THREADS
#include <pthread.h>
#endif

#if !defined(C11_THREADS) && !defined(_WIN32) && !defined(__unix__)
#error no thread implementation available
#endif


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
	int res = mtx_init(&mutex->mutex, mtx_plain);
	if (res != thrd_success)
		return 1;
	return 0;
#endif
#ifdef WIN32_THREADS
	mutex->mutex = CreateMutexA(0, 0, 0);
	if (!mutex->mutex)
		return 1;
	return 0;
#endif
#ifdef POSIX_THREADS
	int res = pthread_mutex_init(&mutex->mutex, 0);
	if (res)
		return 1;
	return 0;
#endif
}

inline char csm_destroy(CS_Mutex* mutex)
{
#ifdef C11_THREADS
	mtx_destroy(&mutex->mutex);
	memset(mutex, '\0', sizeof(CS_Mutex));
	return 0;
#endif
#ifdef WIN32_THREADS
	BOOL res = CloseHandle(mutex->mutex);
	if (!res)
		return 1;
	return 0;
#endif
#ifdef POSIX_THREADS
	int res = pthread_mutex_destroy(&mutex->mutex);
	if (res)
		return 1;
	return 0;
#endif
}

inline char csm_lock(CS_Mutex* mutex)
{
#ifdef C11_THREADS
	int res = mtx_lock(&mutex->mutex);
	if (res != thrd_success)
		return 1;
	return 0;
#endif
#ifdef WIN32_THREADS
	DWORD res = WaitForSingleObject(mutex->mutex, INFINITE);
	if (res)
		return 1;
	return 0;
#endif
#ifdef POSIX_THREADS
	int res = pthread_mutex_lock(&mutex->mutex);
	if (res)
		return 1;
	return 0;
#endif
}

inline char csm_try_lock(CS_Mutex* mutex)
{
#ifdef C11_THREADS
	int res = mtx_trylock(&mutex->mutex);
	if (res != thrd_success)
		return 1;
	return 0;
#endif
#ifdef WIN32_THREADS
	DWORD res = WaitForSingleObject(mutex->mutex, 0);
	if (res)
		return 1;
	return 0;
#endif
#ifdef POSIX_THREADS
	int res = pthread_mutex_trylock(&mutex->mutex);
	if (res)
		return 1;
	return 0;
#endif
}

char csm_unlock(CS_Mutex* mutex)
{
#ifdef C11_THREADS
	int res = mtx_unlock(&mutex->mutex);
	if (res != thrd_success)
		return 1;
	return 0;
#endif
#ifdef WIN32_THREADS
	BOOL res = ReleaseMutex(mutex->mutex);
	if (!res)
		return 1;
	return 0;
#endif
#ifdef POSIX_THREADS
	int res = pthread_mutex_unlock(&mutex->mutex);
	if (res)
		return 1;
	return 0;
#endif
}