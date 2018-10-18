/*************************************************************************
 > File Name: test.c
 > Author:  jinshaohui
 > Mail:    jinshaohui789@163.com
 > Time:    18-10-19
 > Desc:    
 ************************************************************************/
#include<stdio.h>
#include<stdlib.h>
#include"./threadpool.h"

void func(void *arg)
{

	int i = 0;
	for (i = 0; i < 10000000; i ++)
	{

	}

	printf("\r\n args = %u",(int)arg);
	return;
}

int main()
{
    task_t task = {func,NULL};
	int i = 0;
    lock_pool_t *pool = NULL;

   pool = pool_create(10,10);

   for (i = 0 ; i < 2; i ++)

   {
	   task.arg = (void*)i;
	   pool_add_task(pool,&task);
   }


   pool_destory(pool);


	return;
}
