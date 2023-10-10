#pragma once

#if __STDC_VERSION__ >= 201112L && !defined(__STDC_NO_THREADS__) && !defined(NO_C11_THREADS)
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
