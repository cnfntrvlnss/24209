/*************************************************************************
    > File Name: myqueue.h
    > Author: ma6174
    > Mail: ma6174@163.com 
    > Created Time: Mon 27 Apr 2015 01:42:44 AM PDT
 ************************************************************************/

#ifndef MYQUEUE_H
#define MYQUEUE_H

#include <pthread.h>
struct node_t {
	void *data;
	struct node_t *next;
};
typedef struct {
	struct node_t *head;
	struct node_t *tail;
	pthread_cond_t cond;
	pthread_mutex_t mut;
}Myqueue;
#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif
void init_myqueue( Myqueue *qu);
void myqueue_put(Myqueue *qu, void *ele);
void myqueue_putend(Myqueue *qu);
void destroy_myqueue(Myqueue *qu);
void *myqueue_take(Myqueue *qu);
void destroy_myqueue(Myqueue *qu);
size_t myqueue_size(Myqueue *qu);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif
#endif
