#pragma once

#include <assert.h>
#include <string.h> // memset

#if __STDC_VERSION__ >= 201112L && !defined(__STDC_NO_THREADS__)
#define C11_THREADS
#include <threads.h>
#endif

#if defined(_WIN32) && !defined(C11_THREADS)
#define WIN32_THREADS
#include <windows.h>
#endif

#if defined(__unix__) && !defined(C11_THREADS)
#define POSIX_THREADs
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
#endif
#ifdef POSIX_THREADs
#endif
} CS_Mutex;

char csm_create(CS_Mutex* mutex);
char csm_destroy(CS_Mutex* mutex);

char csm_lock(CS_Mutex* mutex);
char csm_try_lock(CS_Mutex* mutex);

inline char csm_create(CS_Mutex* mutex)
{
#ifdef C11_THREADS
	int res = mtx_init(mutex->mutex, mtx_plain);
	if (res != thrd_success)
		return 1;
	return 0;
#endif
#ifdef WIN32_THREADS
#endif
#ifdef POSIX_THREADs
#endif
}

inline char csm_destroy(CS_Mutex* mutex)
{
#ifdef C11_THREADS
	mtx_destroy(mutex->mutex);
	memset(mutex, '\0', sizeof(CS_Mutex));
	return 0;
#endif
#ifdef WIN32_THREADS
#endif
#ifdef POSIX_THREADs
#endif
}

inline char csm_lock(CS_Mutex* mutex)
{
#ifdef C11_THREADS
	int res = mtx_lock(mutex->mutex);
	if (res != thrd_success)
		return 1;
	return 0;
#endif
#ifdef WIN32_THREADS
#endif
#ifdef POSIX_THREADs
#endif
}

inline char csm_try_lock(CS_Mutex* mutex)
{
#ifdef C11_THREADS
	int res = mtx_trylock(mutex->mutex);
	if (res != thrd_success)
		return 1;
	return 0;
#endif
#ifdef WIN32_THREADS
#endif
#ifdef POSIX_THREADs
#endif
}