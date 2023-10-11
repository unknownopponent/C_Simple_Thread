#pragma once

#include <string.h> // memset

#include "common.h"
#include "CS_Mutex.h"

typedef struct CS_ConditionVariable {
	
#ifdef C11_THREADS
	CS_Mutex mutex;
	cnd_t cv_handle;
#endif
#ifdef WIN32_THREADS
	SRWLOCK mutex_handle;
	CONDITION_VARIABLE cv_handle;
#endif
#ifdef POSIX_THREADS
	CS_Mutex mutex;
	pthread_cond_t cv_handle;
#endif

} CS_ConditionVariable;

char cscv_create(CS_ConditionVariable* cv);

char cscv_lock_to_wait(CS_ConditionVariable* cv);
char cscv_wait(CS_ConditionVariable* cv);
char cscv_unlock(CS_ConditionVariable* cv);

char cscv_signal(CS_ConditionVariable* cv);

char cscv_destroy(CS_ConditionVariable* cv);

inline char cscv_create(CS_ConditionVariable* cv)
{
#ifdef C11_THREADS
	if (csm_create(&cv->mutex))
	{
		return 1;
	}
	
	return cnd_init(&cv->cv_handle) != thrd_success;
#endif
#ifdef WIN32_THREADS
	InitializeSRWLock(&cv->mutex_handle);
	InitializeConditionVariable(&cv->cv_handle);
	return 0;
#endif
#ifdef POSIX_THREADS
	if (csm_create(&cv->mutex))
	{
		return 1;
	}
	
	return pthread_cond_init(&cv->cv_handle, 0);
#endif
}

inline char cscv_lock_to_wait(CS_ConditionVariable* cv)
{
#if defined(C11_THREADS) || defined(POSIX_THREADS)
	return csm_lock(&cv->mutex);
#elif defined(WIN32_THREADS)
	AcquireSRWLockExclusive(&cv->mutex_handle);
	return 0;
#endif
	
}

inline char cscv_wait(CS_ConditionVariable* cv)
{
#ifdef C11_THREADS
	return cnd_wait(&cv->cv_handle, &cv->mutex.mutex_handle) != thrd_success;
#endif
#ifdef WIN32_THREADS
	return !SleepConditionVariableSRW(&cv->cv_handle, &cv->mutex_handle, INFINITE, 0);
#endif
#ifdef POSIX_THREADS
	return pthread_cond_wait(&cv->cv_handle, &cv->mutex.mutex_handle);
#endif
}

inline char cscv_unlock(CS_ConditionVariable* cv)
{
#if defined(C11_THREADS) || defined(POSIX_THREADS)
	return csm_unlock(&cv->mutex);
#elif defined(WIN32_THREADS)
	ReleaseSRWLockExclusive(&cv->mutex_handle);
	return 0;
#endif
}

inline char cscv_signal(CS_ConditionVariable* cv)
{
#ifdef C11_THREADS
	if (csm_lock(&cv->mutex))
	{
		return 1;
	}
	
	if (cnd_signal(&cv->cv_handle) != thrd_success)
	{
		return 1;
	}
	
	return csm_unlock(&cv->mutex);
#endif
#ifdef WIN32_THREADS
	WakeConditionVariable(&cv->cv_handle);
	return 0;
#endif
#ifdef POSIX_THREADS
	if (csm_lock(&cv->mutex))
	{
		return 1;
	}

	if (pthread_cond_signal(&cv->cv_handle))
	{
		return 1;
	}
	
	return csm_unlock(&cv->mutex);
#endif
}

inline char cscv_destroy(CS_ConditionVariable* cv)
{
#ifdef C11_THREADS
	if (csm_destroy(&cv->mutex))
	{
		return 1;
	}
	
	cnd_destroy(&cv->cv_handle);
	memset(&cv->cv_handle, 0, sizeof(cnd_t));
	
	return 0;
#endif
#ifdef WIN32_THREADS
	memset(&cv->cv_handle, 0, sizeof(CONDITION_VARIABLE));
	memset(&cv->mutex_handle, 0, sizeof(SRWLOCK));
	return 0;
#endif
#ifdef POSIX_THREADS
	if (csm_destroy(&cv->mutex))
	{
		return 1;
	}

	if (pthread_cond_destroy(&cv->cv_handle))
	{
		return 1;
	}
	memset(&cv->cv_handle, 0, sizeof(pthread_cond_t));
	
	return 0;
#endif
}