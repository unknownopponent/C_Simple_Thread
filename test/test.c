
#include "../CS_Thread.h"
#include "../CS_Mutex.h"

#include <stdio.h>
#include <stdlib.h>

typedef struct Dummy
{
	CS_Mutex mutex;
	int test1;
	int test2;

} Dummy;

void dummy_thread_write(Dummy* dummy)
{

	if (!csm_try_lock(&dummy->mutex))
	{
		char* str = "failled to try lock in thread mutex\n";
		fwrite(str, sizeof(char), strlen(str), stderr);
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
		char* str = "failled to unlock in thread mutex\n";
		fwrite(str, sizeof(char), strlen(str), stderr);
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
#ifdef POSIX_THREADs
	puts("using POSIX threads");
#endif

	Dummy test = { 0 };

	if (csm_create(&test.mutex))
	{
		char* str = "failled to create mutex\n";
		fwrite(str, sizeof(char), strlen(str), stderr);
		return 1;
	}

	if (csm_lock(&test.mutex))
	{
		char* str = "failled to lock mutex\n";
		fwrite(str, sizeof(char), strlen(str), stderr);
		return 1;
	}

	CS_Thread thread = { 0 };
	thread.function = dummy_thread_write;
	thread.args = &test;

	if (cst_create(&thread))
	{
		char* str = "failled to create thread\n";
		fwrite(str, sizeof(char), strlen(str), stderr);
		return 1;
	}

	while (!test.test1)
		;

	if (csm_unlock(&test.mutex))
	{
		char* str = "failled to unlock mutex\n";
		fwrite(str, sizeof(char), strlen(str), stderr);
		exit(1);
	}

	int ret = 0;

	if (cst_join(&thread, &ret))
	{
		char* str = "failled to join thread\n";
		fwrite(str, sizeof(char), strlen(str), stderr);
		return 1;
	}

	if (test.test1 != 1)
	{
		fprintf(stderr, "wrong value in memory %d, 1 expected\n", test.test1);
		return 1;
	}
	if (test.test2 != 4)
	{
		char* str = "thread failled to lock mutex\n";
		fwrite(str, sizeof(char), strlen(str), stderr);
		return 1;
	}

	if (ret != 4)
	{
		fprintf(stderr, "wrong return code %d, 4 expected\n", ret);
		return 1;
	}

	if (csm_destroy(&test.mutex))
	{
		puts("failled to destroy mutex");
		return 1;
	}

	puts("thread tests passed");

	return 0;
}