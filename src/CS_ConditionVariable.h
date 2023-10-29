#pragma once

#include <string.h> // memset
#include <assert.h>

#include "common.h"
#include "CS_Mutex.h"

#if defined(C11_THREADS) || defined(POSIX_THREADS)
typedef CS_Mutex CS_ConditionVariableMutex;
#elif defined(WIN32_THREADS)
typedef SRWLOCK CS_ConditionVariableMutex;
#endif

static inline char cscvm_create(CS_ConditionVariableMutex* mutex);
static inline char cscvm_lock(CS_ConditionVariableMutex* mutex);
static inline char cscvm_unlock(CS_ConditionVariableMutex* mutex);
static inline char cscvm_destroy(CS_ConditionVariableMutex* mutex);

static inline char cscvm_create(CS_ConditionVariableMutex* mutex)
{
#if defined(C11_THREADS) || defined(POSIX_THREADS)
	return csm_create(mutex);
#elif defined(WIN32_THREADS)
	InitializeSRWLock(mutex);
	return 0;
#endif
}

static inline char cscvm_lock(CS_ConditionVariableMutex* mutex)
{
#if defined(C11_THREADS) || defined(POSIX_THREADS)
	return csm_lock(mutex);
#elif defined(WIN32_THREADS)
	AcquireSRWLockExclusive(mutex);
	return 0;
#endif
}

static inline char cscvm_unlock(CS_ConditionVariableMutex* mutex)
{
#if defined(C11_THREADS) || defined(POSIX_THREADS)
	return csm_unlock(mutex);
#elif defined(WIN32_THREADS)
	ReleaseSRWLockExclusive(mutex);
	return 0;
#endif
}

static inline char cscvm_destroy(CS_ConditionVariableMutex* mutex)
{
#if defined(C11_THREADS) || defined(POSIX_THREADS)
	return csm_destroy(mutex);
#elif defined(WIN32_THREADS)
	memset(mutex, 0, sizeof(CS_ConditionVariableMutex));
	return 0;
#endif
}

typedef struct CS_ConditionVariable {
	
	CS_ConditionVariableMutex* mutex;
	
#ifdef C11_THREADS
	cnd_t cv_handle;
#endif
#ifdef WIN32_THREADS
	CONDITION_VARIABLE cv_handle;
#endif
#ifdef POSIX_THREADS
	pthread_cond_t cv_handle;
#endif

} CS_ConditionVariable;


static inline char cscv_create(CS_ConditionVariable* cv);

static inline char cscv_wait(CS_ConditionVariable* cv);
static inline char cscv_signal(CS_ConditionVariable* cv);
static inline char cscv_broadcast(CS_ConditionVariable* cv);

static inline char cscv_destroy(CS_ConditionVariable* cv);

static inline char cscv_create(CS_ConditionVariable* cv)
{
#ifdef C11_THREADS
	return cnd_init(&cv->cv_handle) != thrd_success;
#endif
#ifdef WIN32_THREADS
	InitializeConditionVariable(&cv->cv_handle);
	return 0;
#endif
#ifdef POSIX_THREADS
	return pthread_cond_init(&cv->cv_handle, 0);
#endif
}

static inline char cscv_wait(CS_ConditionVariable* cv)
{
	assert(cv->mutex);
#ifdef C11_THREADS
	return cnd_wait(&cv->cv_handle, &cv->mutex->mutex_handle) != thrd_success;
#endif
#ifdef WIN32_THREADS
	return !SleepConditionVariableSRW(&cv->cv_handle, cv->mutex, INFINITE, 0);
#endif
#ifdef POSIX_THREADS
	return pthread_cond_wait(&cv->cv_handle, &cv->mutex->mutex_handle);
#endif
}

static inline char cscv_signal(CS_ConditionVariable* cv)
{
#ifdef C11_THREADS
	return cnd_signal(&cv->cv_handle) != thrd_success;
#endif
#ifdef WIN32_THREADS
	WakeConditionVariable(&cv->cv_handle);
	return 0;
#endif
#ifdef POSIX_THREADS
	return pthread_cond_signal(&cv->cv_handle);
#endif
}

static inline char cscv_broadcast(CS_ConditionVariable* cv)
{
#ifdef C11_THREADS
	return cnd_broadcast(&cv->cv_handle) != thrd_success;
#endif
#ifdef WIN32_THREADS
	WakeAllConditionVariable(&cv->cv_handle);
	return 0;
#endif
#ifdef POSIX_THREADS
	return pthread_cond_broadcast(&cv->cv_handle);
#endif
}

static inline char cscv_destroy(CS_ConditionVariable* cv)
{
#ifdef C11_THREADS
	cnd_destroy(&cv->cv_handle);
	memset(&cv->cv_handle, 0, sizeof(cnd_t));
	return 0;
#endif
#ifdef WIN32_THREADS
	memset(&cv->cv_handle, 0, sizeof(CONDITION_VARIABLE));
	return 0;
#endif
#ifdef POSIX_THREADS
	if (pthread_cond_destroy(&cv->cv_handle))
	{
		return 1;
	}
	memset(&cv->cv_handle, 0, sizeof(pthread_cond_t));
	return 0;
#endif
}