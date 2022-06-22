
#include "../CS_Thread.h"

#include <stdio.h>

void dummy_thread_write(int* location)
{
	*location = 1;

	cst_exit(2);
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

	int test = 0;

	CS_Thread thread = { 0 };
	thread.function = dummy_thread_write;
	thread.args = &test;

	if (cst_create(&thread))
	{
		char* str = "failled to create thread\n";
		fwrite(str, sizeof(char), strlen(str), stderr);
		return 1;
	}

	int ret = 0;

	if (cst_join(&thread, &ret))
	{
		char* str = "failled to join thread\n";
		fwrite(str, sizeof(char), strlen(str), stderr);
		return 1;
	}

	if (test != 1)
	{
		fprintf(stderr, "wrong value in memory %d, 1 expected\n", test);
		return 1;
	}

	if (ret != 2)
	{
		fprintf(stderr, "wrong return code %d, 2 expected\n", ret);
		return 1;
	}

	puts("thread tests passed");

	return 0;
}