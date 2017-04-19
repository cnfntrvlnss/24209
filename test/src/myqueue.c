/*************************************************************************
    > File Name: myQueue.c
    > Author: ma6174
    > Mail: ma6174@163.com 
    > Created Time: Sun 26 Apr 2015 08:59:05 PM PDT
 ************************************************************************/

#include<stdio.h>
#include <stdlib.h>
#include "myqueue.h"

void init_myqueue(Myqueue *qu)
{
	if(qu == NULL){
		return;
	}
	qu->head = NULL;
	qu->tail = NULL;
	int initret = pthread_cond_init(&(qu->cond), NULL);
	if(initret != 0){
		fprintf(stderr, "fail to initialize pthread_cond_t, error number: %d", initret);exit(1);
	}
	initret = pthread_mutex_init(&(qu->mut), NULL);
	if(initret != 0){
		fprintf(stderr, "fail to initialize pthread_mutex_t, error number: %d", initret);exit(1);
	}
}

void myqueue_put(Myqueue *qu, void *ele)
{
	pthread_mutex_lock(&(qu->mut));
	struct node_t *tt = (struct node_t*)malloc(sizeof(struct node_t));
	tt->data = ele;
	tt->next = NULL;
	if(qu->tail == NULL){
		qu->head = qu->tail = tt;
	}
	else{
		qu->tail->next = tt;
		qu->tail = tt;
	}
	pthread_mutex_unlock(&(qu->mut));
	pthread_cond_broadcast(&(qu->cond));
}
void myqueue_putend(Myqueue *qu)
{
	//end anchor: data = NULL
	myqueue_put(qu, NULL);
}

size_t myqueue_size(Myqueue *qu)
{
	pthread_mutex_lock(&(qu->mut));
	int cnt = 0;
	struct node_t * cur = qu->head;
	while(cur != NULL)
	{
		cur = qu->head->next;
		cnt++;
	}
	pthread_mutex_unlock(&(qu->mut));
	return cnt;
}
void *myqueue_take(Myqueue *qu)
{
	void *ret = NULL;
	pthread_mutex_lock(&(qu->mut));
	while(qu->head ==NULL){
		//wait for new data.
		pthread_cond_wait(&(qu->cond), &(qu->mut));
	}
	if(qu->head->data != NULL){
		struct node_t *tt = qu->head;
		if(qu->head == qu->tail) qu->head = qu->tail = NULL;
		else{
			qu->head = qu->head->next;
		}
		ret = tt->data;
		free(tt);
	}
	pthread_mutex_unlock(&(qu->mut));
	return ret;
}
/** 
 *
 */
void destroy_myqueue(Myqueue *qu)
{
	
	while(qu->head != NULL)
	{
		struct node_t *cur = qu->head;
		qu->head = qu->head->next;
		free(cur);
	}
	qu->tail = NULL;
}

