/*************************************************************************
 > File Name: threadpool.c
 > Author:  jinshaohui
 > Mail:    jinshaohui789@163.com
 > Time:    18-10-19
 > Desc:    
 ************************************************************************/
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<pthread.h>
#include<signal.h>
#include<assert.h>
#include"./threadpool.h"

#define debug(level,...)\
	do{\
		if(level <=POOL_DEBUG)\
		{\
			printf("\r\n###threadid[%u],line[%d],%s:",pthread_self(),__LINE__,__FUNCTION__);\
	        printf(__VA_ARGS__);\	
		}\
	}while(0)


lock_pool_t * pool_create(int thread_max_num,int queue_max_size)
{
	int i = 0;
	int res = 0;
	lock_pool_t *pool = NULL;

	/*创建线程池*/
	pool = (lock_pool_t*)malloc(sizeof(lock_pool_t));
	if(pool == NULL)
	{
		debug(POOL_ERROR,"pool malloc failed..\r\n");

		return NULL;
	}

	memset(pool,0,sizeof(lock_pool_t));

	/*线程ID分配空间*/
	pool->threads = ( pthread_t *)malloc(sizeof(pthread_t) * thread_max_num);
	if(pool->threads == NULL)
	{
		pool_free(pool);
        debug(POOL_ERROR,"pool threads malloc failed\r\n");
		return NULL;
	}
	memset(pool->threads,0, sizeof(pthread_t)* thread_max_num);
	/*创建任务队列 并初始化*/
	pool->task_queue.tasks = (task_t *)malloc(sizeof(task_t)*queue_max_size);
	if(pool->task_queue.tasks == NULL)
	{
		pool_free(pool);
        debug(POOL_ERROR,"pool task queue malloc failed\r\n");
		return NULL;
	}
	memset(pool->task_queue.tasks,0, sizeof(task_t)* queue_max_size);
	pool->task_queue.queue_front = 0;
	pool->task_queue.queue_rear = 0;
	pool->task_queue.queue_size = 0;
	pool->task_queue.queue_max_size = queue_max_size;
	
    /*条件变量及互斥锁初始化*/
	if ((pthread_mutex_init(&(pool->lock),NULL) != 0)
		|| (pthread_cond_init(&(pool->queue_is_empty),NULL) != 0)
		|| (pthread_cond_init(&(pool->queue_not_full),NULL) != 0)
		|| (pthread_cond_init(&(pool->queue_not_empty), NULL) != 0))
	{
		pool_free(pool);
        debug(POOL_ERROR,"pool lock mutex add cond init failed\r\n");
		return NULL;
	}

	/*启动多线程*/
    for(i = 0 ; i < thread_max_num; i ++)
	{
         res = pthread_create(&(pool->threads[i]),NULL,pool_thread_func,(void*)pool);
		 if (res != 0)
		 {
             debug(POOL_ERROR,"pool pthread create %d failed\r\n",i);
             pool_destory(pool);
		     return NULL;
		 }
	}

	/*信息初始化*/
	pool->thread_max_num = thread_max_num;
	pool->pool_shutdown = 0;


	return pool;
}

void pool_free(lock_pool_t *pool)
{

	if(pool == NULL)
	{
		return;
	}

	if(pool->threads)
	{
		free(pool->threads);
	}

	if(pool->task_queue.tasks)
	{
		free(pool->task_queue.tasks);
		pthread_mutex_lock(&(pool->lock));
		pthread_mutex_destroy(&(pool->lock));
		pthread_cond_destroy(&(pool->queue_is_empty));
		pthread_cond_destroy(&(pool->queue_not_full));
		pthread_cond_destroy(&(pool->queue_not_empty));
	}

	free(pool);

    return;
}

void pool_destory(lock_pool_t* pool)
{
	int i = 0;
	if (pool == NULL)
	{
		return;
	}
	
	pthread_mutex_lock(&(pool->lock));
	while(!POOL_QUEUE_IS_EMPTY(pool))
	{
		pthread_cond_wait(&(pool->queue_is_empty),&(pool->lock));
	}
	pthread_mutex_unlock(&(pool->lock));

	pool->pool_shutdown = 1;


	pthread_cond_broadcast(&(pool->queue_not_empty));

	for (i = 0; i < pool->thread_max_num; i++)
	{
		pthread_join((pool->threads[i]),NULL);
	}

	pool_free(pool);

	return;
}

void pool_thread_func(void *args)
{
	lock_pool_t *pool = (lock_pool_t *)args;
	task_t ptask = {0};

	while(1)
	{
		pthread_mutex_lock(&(pool->lock));

		while((POOL_QUEUE_IS_EMPTY(pool)) && (!pool->pool_shutdown))
		{
			pthread_cond_broadcast(&(pool->queue_is_empty));
			debug(POOL_DEBUG,"thread %u,is waiting \n",pthread_self());
			pthread_cond_wait(&(pool->queue_not_empty),&(pool->lock));

		}

		/*线程池关闭时*/
		if(pool->pool_shutdown)
		{
			debug(POOL_DEBUG,"thread %u,is exit \n",pthread_self());
			pthread_mutex_unlock(&(pool->lock));
			pthread_exit(NULL);
		}

        ptask.func = pool->task_queue.tasks[pool->task_queue.queue_front].func;
        ptask.arg = pool->task_queue.tasks[pool->task_queue.queue_front++].arg;
		pool->task_queue.queue_front %= pool->task_queue.queue_max_size;
		pool->task_queue.queue_size--;

		/*发送信号 可以添加任务*/
		if ((pool->task_queue.queue_size + 1) == pool->task_queue.queue_max_size)
		{
		    pthread_cond_broadcast(&(pool->queue_not_full));
		}

		pthread_mutex_unlock(&(pool->lock));

		if(ptask.func != NULL)
		{
			ptask.func(ptask.arg);
		}

	}
	pthread_exit(NULL);
	return;
}

int pool_add_task(lock_pool_t *pool,task_t *task)
{
    pthread_mutex_lock(&(pool->lock));

	while((POOL_QUEUE_IS_FULL(pool)) && (pool->pool_shutdown == 0))
	{
		pthread_cond_wait(&(pool->queue_not_full),&(pool->lock));
	}

	if( pool->pool_shutdown)
	{
		pthread_mutex_unlock(&(pool->lock));
		return -1;
	}

	pool->task_queue.tasks[pool->task_queue.queue_rear].func = task->func;
	pool->task_queue.tasks[pool->task_queue.queue_rear++].arg = task->arg;
	pool->task_queue.queue_rear %= pool->task_queue.queue_max_size;
	pool->task_queue.queue_size++;
    
	/*队列为空时，需要唤醒线程*/
	if (pool->task_queue.queue_size == 1)
	{
		pthread_cond_broadcast(&(pool->queue_not_empty));
	}

	pthread_mutex_unlock(&(pool->lock));

	debug(POOL_DEBUG,"\r\n add task %u",(int)task->arg);

    return 0;
}


