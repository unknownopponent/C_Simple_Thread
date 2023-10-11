#pragma once

#ifdef _WIN32
	#include <windows.h>
#elif defined(__linux__)
	#include <unistd.h>
#endif

int cs_cpu_thread_count()
{
#ifdef _WIN32
	SYSTEM_INFO infos = {0};
	GetSystemInfo(&infos);
	return infos.dwNumberOfProcessors;
#elif defined(__linux__)
	return sysconf(_SC_NPROCESSORS_ONLN);
#endif
}