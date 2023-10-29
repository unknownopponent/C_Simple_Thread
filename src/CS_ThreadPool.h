#pragma once

#include "CS_Thread.h"
#include "CS_ConditionVariable.h"

typedef struct CS_ThreadPoolTask
{
	char active;
	size_t index;
	void (*function)(void*);
	void* params;
	
} CS_ThreadPoolTask;

typedef struct CS_ThreadPool CS_ThreadPool;

typedef struct CS_ThreadPoolContext
{
	CS_Thread thread;
	
	char should_stop;
	char sleeping;
	char exit_on_error;
	char stopped;
	
	CS_ThreadPool* pool;
	
} CS_ThreadPoolContext;

struct CS_ThreadPool
{
	//parameters
	size_t max_input_queue_size;
	size_t max_thread_count;
	
	//internal
	size_t index;
	
	CS_ThreadPoolTask* inputs;
	size_t input_allocated_count;
	
	CS_ConditionVariableMutex input_mutex;
	CS_ConditionVariable wait_worker;
	CS_ConditionVariable wait_input;
	
	CS_ThreadPoolContext** threads;
	size_t thread_count;
	
	CS_ThreadPoolTask* outputs;
	size_t output_allocated_count;
	
	CS_ConditionVariableMutex output_mutex;
	CS_ConditionVariable wait_output;
	
};

/*
 * parameters should be specified by the user
 */
char cstp_create(CS_ThreadPool* pool);

/*
 * if a thread is sleeping, a thread is resumed
 * else if the maximum thread count isnt reached, a new thread is created in the pool
 * else if the input queue isnt full, the task is added and a thread will execute later
 * else the calling thread sleep until there is space in the input queue
 */
char cstp_queue(CS_ThreadPool* pool, void** functions, void** params, size_t count);

/*
 * the output queue should be locked before dequeuing
 */
char cstp_lock_output_queue(CS_ThreadPool* pool);

/*
 * if the output queue is empty, the calling thread can wait until a worker thread adds to the output queue
 */
char cstp_wait_output_queue(CS_ThreadPool* pool);

/*
 * wait until every queued task is in the output queue
 */
char cstp_wait_all_output_queue(CS_ThreadPool* pool);

size_t cstp_output_queue_count(CS_ThreadPool* pool);

char cstp_dequeue_output(CS_ThreadPool* pool, CS_ThreadPoolTask* output, size_t count);

void cstp_empty_output_queue(CS_ThreadPool* pool);

char cstp_unlock_output_queue(CS_ThreadPool* pool);

/*
 * release ressources without waiting worker threads to finish executing the input queue tasks
 */
char cstp_destroy(CS_ThreadPool* pool);
