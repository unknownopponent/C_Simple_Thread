
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

typedef struct Dummy
{
	CS_Mutex mutex;
	CS_ConditionVariable cv;
	int test1;
	int test2;

} Dummy;

void dummy_thread_write(Dummy* dummy)
{
	if (!csm_try_lock(&dummy->mutex))
	{
		print_error("try lock succed while fail was expected\n");
		exit(1);
	}
	
	if (cs_sleep_seconds(0.5))
	{
		print_error("failled to sleep\n");
		exit(1);
	}

	if (cscv_signal(&dummy->cv))
	{
		print_error("condition variable signal failled\n");
		exit(1);
	}

	dummy->test1 = 1;

	if (csm_lock(&dummy->mutex))
	{
		dummy->test2 = 3;
		cst_exit(3);
	}

	dummy->test2 = 4;

	if (csm_unlock(&dummy->mutex))
	{
		print_error("failled to unlock in thread mutex\n");
		exit(1);
	}

	puts("thread exit normally");

	cst_exit(4);
}

int main(void)
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

	Dummy test = { 0 };

	if (csm_create(&test.mutex))
	{
		print_error("failled to create mutex\n");
		return 1;
	}

	if (csm_lock(&test.mutex))
	{
		print_error("failled to lock mutex\n");
		return 1;
	}
	
	if (cscv_create(&test.cv))
	{
		print_error("failled to create condition variable\n");
		return 1;
	}
	if (cscv_lock_to_wait(&test.cv))
	{
		print_error("failled to lock condition varaible mutex\n");
		return 1;
	}

	CS_Thread thread = { 0 };
	thread.function = dummy_thread_write;
	thread.args = &test;

	if (cst_create(&thread))
	{
		print_error("failled to create thread\n");
		return 1;
	}

	if (cscv_wait(&test.cv))
	{
		print_error("failled to wait condition variable\n");
		return 1;
	}
	
	if (cscv_unlock(&test.cv))
	{
		print_error("failled to unlock condition variable mutex\n");
		return 1;
	}
	
	if (cs_sleep_seconds(0.5))
	{
		print_error("failled to sleep\n");
		return 1;
	}

	if (csm_unlock(&test.mutex))
	{
		print_error("failled to unlock mutex\n");
		return 1;
	}
	
	int ret = 0;

	if (cst_join(&thread, &ret))
	{
		print_error("failled to join thread\n");
		return 1;
	}

	if (test.test1 != 1)
	{
		fprintf(stderr, "wrong value in memory %d, 1 expected\n", test.test1);
		return 1;
	}
	if (test.test2 != 4)
	{
		print_error("thread failled to lock mutex\n");
		return 1;
	}

	if (ret != 4)
	{
		fprintf(stderr, "wrong return code %d, 4 expected\n", ret);
		return 1;
	}

	if (csm_destroy(&test.mutex))
	{
		print_error("failled to destroy mutex");
		return 1;
	}

	puts("thread tests passed");

	return 0;
}