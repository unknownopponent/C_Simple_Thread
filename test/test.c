
#include "CS_Thread.h"
#include "CS_Mutex.h"
#include "CS_ConditionVariable.h"
#include "CS_Sleep.h"
#include "CS_Utils.h"
#include "CS_ThreadPool.h"

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
	thread.function = (void (*)(void))thread_write_int;
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
	ThreadMutexContext ctx = { 0 };
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
	thread.function = (void (*)(void))thread_mutex_write;
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
	TreadSignalContext ctx = { 0 };

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
	thread.function = (void (*)(void))thread_signal;
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

void thread_pool_write(int* integer)
{
	if (cs_sleep_seconds(0.2))
	{
		print_error("failled to sleep\n");
		*integer = -1;
		return;
	}
	*integer = 1;
}

char test_thread_pool()
{
	size_t cpus = cs_cpu_thread_count();

	CS_ThreadPool pool = { 0 };
	pool.max_input_queue_size = cpus;
	pool.max_thread_count = cpus;

	if (cstp_create(&pool))
	{
		print_error("thread pool creation failled\n");
		return 1;
	}

	int* integers = malloc(cpus * sizeof(int) * 3);
	if (!integers)
	{
		print_error("integers allocation failled\n");
		return 1;
	}
	memset(integers, 0, cpus * sizeof(int) * 3);

	int** integer_pointers = malloc(cpus * sizeof(int*) * 3);
	if (!integer_pointers)
	{
		print_error("integer pointers allocation failled\n");
		return 1;
	}
	for (size_t i = 0; i < cpus * 3; i++)
	{
		integer_pointers[i] = integers + i;
	}

	void** functions = malloc(cpus * sizeof(void*) * 3);
	if (!functions)
	{
		print_error("functions allocation failled\n");
		return 1;
	}
	for (size_t i = 0; i < cpus * 3; i++)
	{
		functions[i] = (void (*)(void))thread_pool_write;
	}

	if (cstp_queue(&pool, functions, (void**)integer_pointers, cpus * 3))
	{
		print_error("thread pool input queue failled\n");
		return 1;
	}

	if (cstp_lock_output_queue(&pool))
	{
		print_error("thread pool output queue lock failled\n");
		return 1;
	}

	if (cstp_wait_all_output_queue(&pool))
	{
		print_error("thread pool wait all output queue failled\n");
		return 1;
	}

	if (cstp_unlock_output_queue(&pool))
	{
		print_error("thread pool output queue unlock failled\n");
		return 1;
	}

	if (cstp_queue(&pool, functions, (void**)integer_pointers, cpus * 3))
	{
		print_error("thread pool input queue failled\n");
		return 1;
	}

	CS_ThreadPoolTask* tasks = malloc(cpus * 3 * 2 * sizeof(CS_ThreadPoolTask));
	if (!tasks)
	{
		print_error("tasks allocation failled\n");
		return 1;
	}

	if (cstp_lock_output_queue(&pool))
	{
		print_error("thread pool output queue lock failled\n");
		return 1;
	}

	size_t dequeud_count = 0;

	while (dequeud_count != cpus * 3 * 2)
	{
		size_t tmp = cstp_output_queue_count(&pool);
		while (!tmp)
		{
			if (cstp_wait_output_queue(&pool))
			{
				print_error("wait output queue failled\n");
				return 1;
			}
			tmp = cstp_output_queue_count(&pool);
		}

		if (cstp_dequeue_output(&pool, tasks, tmp))
		{
			print_error("wait output queue failled\n");
			return 1;
		}

		printf("dequeued %d tasks\n", (int)tmp);

		for (size_t i = 0; i < tmp; i++)
		{
			int j = *(int*)tasks[i].params;
			if (j != 1)
			{
				fprintf(stderr, "wrong task result %d\n", j);
				return 1;
			}
		}

		dequeud_count += tmp;
	}

	if (cstp_unlock_output_queue(&pool))
	{
		print_error("thread pool output queue unlock failled\n");
		return 1;
	}

	if (cstp_destroy(&pool))
	{
		print_error("thread pool destruction failled\n");
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
	puts("thread test passed");
	if (test_mutex())
	{
		print_error("mutex test failled\n");
		return 1;
	}
	puts("mutex test passed");
	if (test_condition_variable())
	{
		print_error("condition variable test failled\n");
		return 1;
	}
	puts("condition variable test passed");
	if (test_thread_pool())
	{
		print_error("thread pool test failled\n");
		return 1;
	}
	puts("thread pool test passed");

	puts("tests passed succesfully\n");

	return 0;
}