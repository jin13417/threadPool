/*************************************************************************
 > File Name: threadpool.h
 > Author:  jinshaohui
 > Mail:    jinshaohui789@163.com
 > Time:    18-10-19
 > Desc:    
 ************************************************************************/

#ifndef _THREAD_POOL_LOCK_H_
#define _THREAD_POOL_LOCK_H_

/*任务*/
typedef struct _task_t
{
	void (*func)(void *arg);
    void *arg;
	struct _task_t *next;
}task_t;

typedef struct _task_queue_t
{
	task_t    *tasks;             /*任务队列*/
    int       queue_front;             /*队头*/
    int       queue_rear;              /*队尾*/
	int       queue_size;              /*队列当前的大小*/
    int       queue_max_size;          /*队列容纳的最大任务数*/
}task_queue_t;

typedef struct _lock_pool_t
{
	pthread_mutex_t  lock;             /*锁住整个结构体*/
	pthread_cond_t   queue_is_empty;   /*线程池为空时*/
	pthread_cond_t   queue_not_full;   /*任务队列不为满时*/
    pthread_cond_t   queue_not_empty;  /*任务队列不为空时*/

	pthread_t *threads;                /*存放线程ID*/
    

	/*线程池信息*/
	int thread_max_num;               /*最大线程数*/
	int pool_shutdown;                /*线程池退出*/

    task_queue_t task_queue;
}lock_pool_t;

enum 
{
	POOL_ERROR,
	POOL_WARNING,
	POOL_INFO,
	POOL_DEBUG
};


#define POOL_QUEUE_IS_EMPTY(pool) (0 == pool->task_queue.queue_size)
#define POOL_QUEUE_IS_FULL(pool) (pool->task_queue.queue_max_size == pool->task_queue.queue_size)


lock_pool_t * pool_create(int thread_max_num,int queue_max_size);
void pool_destory(lock_pool_t* pool);
void pool_thread_func(void *args);
int pool_add_task(lock_pool_t *pool,task_t *task);

#endif
