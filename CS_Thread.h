#pragma once

#include <assert.h>
#include <string.h> // memset
#include <stdio.h>

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

typedef struct CS_Thread
{
	void(*function);
	void* args;

#ifdef C11_THREADS
	thrd_t thread_handle;
#endif
#ifdef WIN32_THREADS
	HANDLE thread_handle;
#endif
#ifdef POSIX_THREADS
	pthread_t thread_handle;
#endif

} CS_Thread;


char cst_create(CS_Thread* thread);
char cst_join(CS_Thread* thread, int* return_code);

void cst_exit(int ret);


inline char cst_create(CS_Thread* thread)
{
	assert(thread->function != 0);

#ifdef C11_THREADS
	if (thrd_create(&thread->thread_handle, thread->function, thread->args) != thrd_success)
		return 1;
	return 0;
#endif
#ifdef WIN32_THREADS
	thread->thread_handle = CreateThread(0, 0, (LPTHREAD_START_ROUTINE)thread->function, thread->args, 0, 0);

	if (thread->thread_handle)
		return 0;
	return 1;
#endif
#ifdef POSIX_THREADS
	if (pthread_create(&thread->thread_handle, 0, thread->function, thread->args))
		return 1;
	return 0;
#endif
}

inline char cst_join(CS_Thread* thread, int* return_code)
{
	assert(thread->thread_handle);

#ifdef C11_THREADS
	int res = thrd_join(thread->thread_handle, return_code);
	if (res != thrd_success)
	{
		assert(0);
		return 1;
	}

	memset(&thread->thread_handle, '\0', sizeof(thrd_t));

	return 0;
#endif
#ifdef WIN32_THREADS
	
	DWORD res = WaitForSingleObject(thread->thread_handle, INFINITE);
	if (res)
	{
		assert(0);
		return 1;
	}

	DWORD ret;
	BOOL res2 = GetExitCodeThread(thread->thread_handle, &ret);
	if (!res2)
	{
		assert(0);
		return 1;
	}

	res2 = CloseHandle(thread->thread_handle);
	if (res)
	{
		assert(0);
		return 1;
	}

	memset(&thread->thread_handle, '\0', sizeof(HANDLE));

	*return_code = (int)ret;

	return 0;
#endif
#ifdef POSIX_THREADS
	void* ret;
	int res = pthread_join(thread->thread_handle, &ret);
	if (res)
	{
		assert(0);
		return 1;
	}

	memset(&thread->thread_handle, '\0', sizeof(pthread_t));

	*return_code = (int)ret;

	return 0;
#endif
}

inline void cst_exit(int ret)
{
#ifdef C11_THREADS
	thrd_exit(ret);
#endif
#ifdef WIN32_THREADS
	ExitThread(ret);
#endif
#ifdef POSIX_THREADS
	pthread_exit((void*)ret);
#endif
}
