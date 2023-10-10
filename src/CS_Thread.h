#pragma once

#include <assert.h>
#include <string.h> // memset
#include <stdint.h> // intptr_t

#include "common.h"

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
	return thrd_create(&thread->thread_handle, thread->function, thread->args) != thrd_success;
#endif
#ifdef WIN32_THREADS
	thread->thread_handle = CreateThread(0, 0, (LPTHREAD_START_ROUTINE)thread->function, thread->args, 0, 0);

	return thread->thread_handle == 0;
#endif
#ifdef POSIX_THREADS
	return pthread_create(&thread->thread_handle, 0, thread->function, thread->args);
#endif
}

inline char cst_join(CS_Thread* thread, int* return_code)
{
	assert(thread->thread_handle);

#ifdef C11_THREADS
	int res = thrd_join(thread->thread_handle, return_code);
	if (res != thrd_success)
	{
		return 1;
	}

	memset(&thread->thread_handle, 0, sizeof(thrd_t));

	return 0;
#endif
#ifdef WIN32_THREADS
	
	DWORD res = WaitForSingleObject(thread->thread_handle, INFINITE);
	if (res)
	{
		return 1;
	}

	DWORD ret;
	BOOL res2 = GetExitCodeThread(thread->thread_handle, &ret);
	if (!res2)
	{
		return 1;
	}

	res2 = CloseHandle(thread->thread_handle);
	if (!res2)
	{
		return 1;
	}

	memset(&thread->thread_handle, 0, sizeof(HANDLE));

	*return_code = (int)ret;

	return 0;
#endif
#ifdef POSIX_THREADS
	void* ret;
	int res = pthread_join(thread->thread_handle, &ret);
	if (res)
	{
		return 1;
	}

	memset(&thread->thread_handle, 0, sizeof(pthread_t));

	*return_code = (int)(intptr_t)ret;

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
	pthread_exit((void*)(intptr_t)ret);
#endif
}
