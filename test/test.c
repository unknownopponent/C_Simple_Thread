
#include "CS_Thread.h"
#include "CS_Mutex.h"
#include "CS_ConditionVariable.h"
#include "CS_Sleep.h"
#include "CS_Utils.h"

#include <stdio.h>
#include <stdlib.h>

void print_error(char* str)
{
	fwrite(str, sizeof(char), strlen(str), stderr);
}

void thread_write_int(int* integer)
{
	*integer = 2;
	cst_exit(3);
}

char test_thread()
{
	int test = -1;
	
	CS_Thread thread = { 0 };
	thread.function = thread_write_int;
	thread.args = &test;
	
	if (cst_create(&thread))
	{
		print_error("failled to create thread\n");
		return 1;
	}
	
	int ret = -1;
	
	if (cst_join(&thread, &ret))
	{
		print_error("failled to join thread\n");
		return 1;
	}
	
	if (test != 2)
	{
		fprintf(stderr, "thread did not write expected value %d\n", test);
		return 1;
	}
	
	if (ret != 3)
	{
		fprintf(stderr, "thread did not return expected value %d\n", ret);
		return 1;
	}
	
	return 0;
}

typedef struct ThreadMutexContext
{
	CS_Mutex mutex;
	int integer;
} ThreadMutexContext;

void thread_mutex_write(ThreadMutexContext* ctx)
{
	if (csm_lock(&ctx->mutex))
	{
		print_error("thread failled to lock mutex\n");
		cst_exit(1);
	}

	ctx->integer = 3;

	if (csm_unlock(&ctx->mutex))
	{
		print_error("thread failled to unlock mutex\n");
		cst_exit(1);
	}
	
	cst_exit(0);
}

char test_mutex()
{
	ThreadMutexContext ctx = {0};
	ctx.integer = -1;
	
	if (csm_create(&ctx.mutex))
	{
		print_error("failled to create mutex\n");
		return 1;
	}
	
	if (csm_lock(&ctx.mutex))
	{
		print_error("failled to lock mutex\n");
		return 1;
	}

	CS_Thread thread = { 0 };
	thread.function = thread_mutex_write;
	thread.args = &ctx;
	
	if (cst_create(&thread))
	{
		print_error("failled to create thread\n");
		return 1;
	}
	
	if (cs_sleep_seconds(0.2))
	{
		print_error("failled to sleep\n");
		return 1;
	}
	
	ctx.integer = 3;
	
	if (csm_unlock(&ctx.mutex))
	{
		print_error("failled to unlock mutex\n");
		return 1;
	}
	
	int ret = -1;
	
	if (cst_join(&thread, &ret))
	{
		print_error("failled to join thread\n");
		return 1;
	}
	
	if (ret != 0)
	{
		fprintf(stderr, "thread did not return expected value %d\n", ret);
		return 1;
	}
	
	if (ctx.integer != 3)
	{
		fprintf(stderr, "thread did not write expected value %d\n", ctx.integer);
		return 1;
	}
	
	if (csm_destroy(&ctx.mutex))
	{
		print_error("failled to destroy mutex\n");
		return 1;
	}
	
	return 0;
}

typedef struct TreadSignalContext
{
	CS_ConditionVariableMutex mutex;
	CS_ConditionVariable cv;
} TreadSignalContext;

void thread_signal(TreadSignalContext* ctx)
{
	if (cs_sleep_seconds(0.2))
	{
		print_error("failled to sleep\n");
		cst_exit(1);
	}
	
	if (cscvm_lock(&ctx->mutex))
	{
		print_error("thread failled to lock mutex\n");
		cst_exit(1);
	}

	if (cscv_signal(&ctx->cv))
	{
		print_error("thread failled to signal condition variable\n");
		cst_exit(1);
	}

	if (cscvm_unlock(&ctx->mutex))
	{
		print_error("thread failled to unlock mutex\n");
		cst_exit(1);
	}

	cst_exit(0);
}

char test_condition_variable()
{
	TreadSignalContext ctx = {0};
	
	if (cscvm_create(&ctx.mutex))
	{
		print_error("failled to create mutex\n");
		return 1;
	}
	
	if (cscv_create(&ctx.cv))
	{
		print_error("failled to create condition variable\n");
		return 1;
	}
	
	ctx.cv.mutex = &ctx.mutex;
	
	if (cscvm_lock(&ctx.mutex))
	{
		print_error("failled to lock mutex\n");
		return 1;
	}
	
	CS_Thread thread = { 0 };
	thread.function = thread_signal;
	thread.args = &ctx;
	
	if (cst_create(&thread))
	{
		print_error("failled to create thread\n");
		return 1;
	}
	
	if (cscv_wait(&ctx.cv))
	{
		print_error("failled to wait condition variable\n");
		return 1;
	}
	
	if (cscvm_unlock(&ctx.mutex))
	{
		print_error("failled to unlock mutex\n");
		return 1;
	}
	
	int ret = -1;
	
	if (cst_join(&thread, &ret))
	{
		print_error("failled to join thread\n");
		return 1;
	}
	
	if (ret != 0)
	{
		fprintf(stderr, "thread did not return expected value %d\n", ret);
		return 1;
	}
	
	if (cscv_destroy(&ctx.cv))
	{
		print_error("failled to destroy condition variable\n");
		return 1;
	}
	
	if (cscvm_destroy(&ctx.mutex))
	{
		print_error("failled to destroy mutex\n");
		return 1;
	}
	
	return 0;
}

int main()
{
#ifdef C11_THREADS
	puts("using C11 threads");
#endif
#ifdef WIN32_THREADS
	puts("using WIN32 threads");
#endif
#ifdef POSIX_THREADS
	puts("using POSIX threads");
#endif

	printf("%d cpu threads\n", cs_cpu_thread_count());
	
	if (test_thread())
	{
		print_error("thread test failled\n");
		return 1;
	}
	if (test_mutex())
	{
		print_error("mutex test failled\n");
		return 1;
	}
	if (test_condition_variable())
	{
		print_error("condition variable test failled\n");
		return 1;
	}
	
	puts("tests passed succesfully\n");
	
	return 0;
}