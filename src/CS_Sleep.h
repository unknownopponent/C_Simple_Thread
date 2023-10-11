#pragma once

#include <stdint.h>
#ifdef _WIN32
#include <windows.h>
#elif defined(__linux__)
#include <time.h>
#endif

char cs_sleep_seconds(double seconds);

inline char cs_sleep_seconds(double seconds)
{
#ifdef __linux__
	struct timespec ts = {0};
	ts.tv_sec = (uint64_t)seconds;
	ts.tv_nsec = ((uint64_t)(seconds * 10e8)) % (uint64_t)10e8;
	return nanosleep(&ts, 0);
#elif defined(_WIN32)
	Sleep(seconds * 10e2);
	return 0;
#endif
}