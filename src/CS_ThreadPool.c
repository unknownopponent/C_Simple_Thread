
#include <stdlib.h>
#include <string.h>

#include "CS_ThreadPool.h"

char cstp_create(CS_ThreadPool* pool)
{
	if (pool->max_input_queue_size == 0)
	{
		return 1;
	}
	if (pool->max_thread_count == 0)
	{
		return 1;
	}
	
	pool->index = 0;
	
	pool->inputs = 0;
	pool->input_allocated_count = 0;
	
	if (cscvm_create(&pool->input_mutex))
	{
		return 1;
	}
	if (cscv_create(&pool->wait_worker))
	{
		return 1;
	}
	pool->wait_worker.mutex = &pool->input_mutex;
	if (cscv_create(&pool->wait_input))
	{
		return 1;
	}
	pool->wait_input.mutex = &pool->input_mutex;
	
	pool->threads = 0;
	pool->thread_count = 0;
	
	pool->outputs = 0;
	pool->output_allocated_count = 0;
	
	if (cscvm_create(&pool->output_mutex))
	{
		return 1;
	}
	if (cscv_create(&pool->wait_output))
	{
		return 1;
	}
	pool->wait_output.mutex = &pool->output_mutex;
	
	return 0;
}

char cstp_push_queue(CS_ThreadPool* pool, void* function, void* params)
{
	for (size_t i=0; i<pool->input_allocated_count; i++)
	{
		if (!pool->inputs[i].active)
		{
			pool->inputs[i].active = 1;
			pool->inputs[i].index = pool->index;
			pool->index += 1;
			pool->inputs[i].function = function;
			pool->inputs[i].params = params;
			return 0;
		}
	}
	
	//increase input
	void* tmp = realloc(pool->inputs, sizeof(CS_ThreadPoolTask) * (pool->input_allocated_count +1));
	if (!tmp)
	{
		return 1;
	}
	
	pool->inputs = tmp;
	pool->inputs[pool->input_allocated_count].active = 1;
	pool->inputs[pool->input_allocated_count].index = pool->index;
	pool->index += 1;
	pool->inputs[pool->input_allocated_count].function = function;
	pool->inputs[pool->input_allocated_count].params = params;
	
	pool->input_allocated_count += 1;
	
	return 0;
}

void cstp_thread_handler(CS_ThreadPoolContext* ctx)
{
	CS_ThreadPoolTask task;
	
	while (!ctx->should_stop)
	{
		if (cscvm_lock(&ctx->pool->input_mutex))
		{
			goto error;
		}
		
		while (1)
		{
			if (ctx->should_stop)
			{
				if (cscvm_unlock(&ctx->pool->input_mutex))
				{
					ctx->exit_on_error = 1;
				}
				ctx->sleeping = 0;
				ctx->stopped = 1;
				return;
			}
			
			for (size_t i=0;i<ctx->pool->input_allocated_count; i++)
			{
				if (ctx->pool->inputs[i].active)
				{
					memcpy(&task, ctx->pool->inputs + i, sizeof(CS_ThreadPoolTask));
					ctx->pool->inputs[i].active = 0;
					ctx->sleeping = 0;
					if (cscv_signal(&ctx->pool->wait_worker))
					{
						cscvm_unlock(&ctx->pool->input_mutex);
						goto error;
					}
					goto process;
				}
			}
			
			ctx->sleeping = 1;
			if (cscv_wait(&ctx->pool->wait_input))
			{
				cscvm_unlock(&ctx->pool->input_mutex);
				ctx->sleeping = 0;
				goto error;
			}
		}
		
		process:
		if (cscvm_unlock(&ctx->pool->input_mutex))
		{
			goto error;
		}
		
		task.function(task.params);
		
		if (cscvm_lock(&ctx->pool->output_mutex))
		{
			goto error;
		}
		
		for (size_t i=0; i<ctx->pool->output_allocated_count; i++)
		{
			if (!ctx->pool->outputs[i].active)
			{
				memcpy(ctx->pool->outputs + i, &task, sizeof(CS_ThreadPoolTask));
				goto end_output;
			}
		}
		
		void* tmp = realloc(ctx->pool->outputs, sizeof(CS_ThreadPoolTask) * (ctx->pool->output_allocated_count +1));
		if (!tmp)
		{
			cscvm_unlock(&ctx->pool->output_mutex);
			goto error;	
		}
		ctx->pool->outputs = tmp;
		
		memcpy(ctx->pool->outputs + ctx->pool->output_allocated_count, &task, sizeof(CS_ThreadPoolTask));
		ctx->pool->output_allocated_count += 1;
		
		end_output:
		
		if (cscv_signal(&ctx->pool->wait_output))
		{
			cscvm_unlock(&ctx->pool->output_mutex);
			goto error;	
		}
		
		if (cscvm_unlock(&ctx->pool->output_mutex))
		{
			goto error;
		}
	}
	
	ctx->stopped = 1;
	return;
	
	error:
	ctx->exit_on_error = 1;
	ctx->stopped = 1;
}

char cstp_queue(CS_ThreadPool* pool, void** functions, void** params, size_t count)
{
	if (cscvm_lock(&pool->input_mutex))
	{
		return 1;
	}
	
	//wake threads
	size_t param_index=0;
	size_t thread_index=0;
	for (; param_index<count;param_index++)
	{
		for (; thread_index<pool->thread_count;thread_index++)
		{
			if (pool->threads[thread_index]->sleeping)
			{
				if (cstp_push_queue(pool, functions[param_index], params[param_index]))
				{
					goto fail;
				}
				if (cscv_signal(&pool->wait_input))
				{
					goto fail;
				}
				if (param_index == count)
				{
					goto success;
				}
				thread_index += 1;
				goto continue_thread_wake_loop;
			}
		}
		break;
		continue_thread_wake_loop:
		;
	}
	
	//start new thread
	for (; param_index<count;param_index++)
	{
		if (pool->thread_count >= pool->max_thread_count)
		{
			break;
		}
		
		if (cstp_push_queue(pool, functions[param_index], params[param_index]))
		{
			goto fail;
		}
		
		void* tmp = realloc(pool->threads, sizeof(CS_ThreadPoolContext*)*(pool->thread_count +1));
		if (!tmp)
		{
			goto fail;
		}
		
		pool->threads = tmp;
		pool->threads[pool->thread_count] = malloc(sizeof(CS_ThreadPoolContext));
		if (!pool->threads[pool->thread_count])
		{
			goto fail;
		}

		memset(pool->threads[pool->thread_count], 0, sizeof(CS_ThreadPoolContext));
		pool->threads[pool->thread_count]->thread.function = (void (*)(void))cstp_thread_handler;
		pool->threads[pool->thread_count]->thread.args = pool->threads[pool->thread_count];
		pool->threads[pool->thread_count]->pool = pool;
		
		if (cst_create(&pool->threads[pool->thread_count]->thread))
		{
			goto fail;
		}

		pool->thread_count += 1;
	}
	
	if (param_index == count)
	{
		goto success;
	}
	
	//add input queue
	do
	{
		for (size_t i=0; i<pool->input_allocated_count; i++)
		{
			if (!pool->inputs[i].active)
			{
				if (cstp_push_queue(pool, functions[param_index], params[param_index]))
				{
					goto fail;
				}
				param_index += 1;
				if (param_index == count)
				{
					goto success;
				}
				goto continue_enqueue_loop;
			}
		}
		
		//wait a thread to dequeue inputs
		if (cscv_wait(&pool->wait_worker))
		{
			goto fail;
		}
		
		continue_enqueue_loop:
		;
	} while (1);
	
	fail:
	
	if (cscvm_unlock(&pool->input_mutex))
	{
		return 1;
	}
	
	return 1;
	
	success:
	
	if (cscvm_unlock(&pool->input_mutex))
	{
		return 1;
	}
	
	return 0;
}

char cstp_lock_output_queue(CS_ThreadPool* pool)
{
	return cscvm_lock(&pool->output_mutex);
}

char cstp_wait_output_queue(CS_ThreadPool* pool)
{
	do
	{
		if (cscv_wait(&pool->wait_output))
		{
			return 1;
		}
		
		for (size_t i=0; i<pool->output_allocated_count; i++)
		{
			if (pool->outputs[i].active)
			{
				return 0;
			}
		}
	} while (1);
}

char cstp_wait_all_output_queue(CS_ThreadPool* pool)
{
	do
	{
		char all_sleeping = 1;
		for (size_t i=0; i<pool->thread_count;i++)
		{
			if (!pool->threads[i]->sleeping)
			{
				all_sleeping = 0;
				break;
			}
		}
		if (all_sleeping)
		{
			return 0;
		}
	} while (1);
}

size_t cstp_output_queue_count(CS_ThreadPool* pool)
{
	size_t count = 0;
	
	for (size_t i=0; i<pool->output_allocated_count; i++)
	{
		if (pool->outputs[i].active)
		{
			count += 1;
		}
	}
	
	return count;
}

char cstp_dequeue_output(CS_ThreadPool* pool, CS_ThreadPoolTask* output, size_t count)
{
	size_t dequeued_count = 0;
	size_t i = 0;
	while (i < pool->output_allocated_count && dequeued_count != count)
	{
		if (pool->outputs[i].active)
		{
			memcpy(output + dequeued_count, pool->outputs + i, sizeof(CS_ThreadPoolTask));
			dequeued_count += 1;
			pool->outputs[i].active = 0;
		}
		i += 1;
	}
	
	return dequeued_count != count;
}

void cstp_empty_output_queue(CS_ThreadPool* pool)
{
	for (size_t i=0; i<pool->output_allocated_count; i++)
	{
		pool->outputs[i].active = 0;
	}
}


char cstp_unlock_output_queue(CS_ThreadPool* pool)
{
	return cscvm_unlock(&pool->output_mutex);
}

char cstp_destroy(CS_ThreadPool* pool)
{
	//stop threads
	if (cscvm_lock(&pool->input_mutex))
	{
		return 1;
	}
	
	for (size_t i=0; i<pool->thread_count; i++)
	{
		pool->threads[i]->should_stop = 1;
	}
	
	if (cscv_broadcast(&pool->wait_input))
	{
		cscvm_unlock(&pool->input_mutex);
		return 1;
	}

	if (cscvm_unlock(&pool->input_mutex))
	{
		return 1;
	}
	
	int ret;
	for (size_t i=0; i<pool->thread_count; i++)
	{
		if (cst_join(&pool->threads[i]->thread, &ret))
		{
			cscvm_unlock(&pool->input_mutex);
			return 1;
		}
	}
	
	//free
	if (pool->inputs)
	{
		free(pool->inputs);
	}
	
	if (cscv_destroy(&pool->wait_worker))
	{
		return 1;
	}
	if (cscv_destroy(&pool->wait_input))
	{
		return 1;
	}
	if (cscvm_destroy(&pool->input_mutex))
	{
		return 1;
	}
	
	if (pool->threads)
	{
		free(pool->threads);
	}
	
	if (pool->outputs)
	{
		free(pool->outputs);
	}
	
	if (cscv_destroy(&pool->wait_output))
	{
		return 1;
	}
	if (cscvm_destroy(&pool->output_mutex))
	{
		return 1;
	}
	
	memset(pool, 0, sizeof(CS_ThreadPool));
	
	return 0;
}