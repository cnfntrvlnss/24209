/*************************************************************************
	> File Name: utilities/mympmc.cpp
	> Author: 
	> Mail: 
	> Created Time: Tue 22 Nov 2016 04:14:25 AM EST
 ************************************************************************/

#include "mympmc.h"
#include<iostream>
using namespace std;

////////////////////////////////////////////////
unsigned mmq_size(MMQ_T *pqu)
{
    unsigned cnt = 0;
    pthread_mutex_lock(&pqu->headMut);
    if(pqu->head != 0){
        pthread_mutex_lock(&pqu->tailMut);
        E_T *curNode = pqu->head;
        while(curNode != pqu->tail){
            cnt ++; 
            curNode = curNode->next;
		}
		cnt ++;
		pthread_mutex_unlock(&pqu->tailMut);
	}
    pthread_mutex_unlock(&pqu->headMut);
    return cnt;
}

void mmq_put(MMQ_T *pqu, void *pdata)
{
    E_T *ele = (E_T*)malloc(sizeof(E_T));
    ele->data = pdata;
    ele->next = 0;
    pthread_mutex_lock(&pqu->headMut);
    pthread_mutex_lock(&pqu->tailMut);
    if(pqu->head == 0){
        pqu->head = pqu->tail = ele;
        pthread_mutex_unlock(&pqu->headMut);
        pthread_mutex_unlock(&pqu->tailMut);
        pthread_cond_broadcast(&pqu->headCnd);
        return;
    }
		    
    pthread_mutex_unlock(&pqu->headMut);
    pqu->tail->next = ele;
    pqu->tail = ele;
    pthread_mutex_unlock(&pqu->tailMut);
}

static void mmq_take_unlocked(MMQ_T *pqu, void **ppdata)
{
    E_T *ele = 0;
    pthread_mutex_lock(&pqu->tailMut);
    if(pqu->head == pqu->tail){
        ele = pqu->head;
        pqu->head = pqu->tail = 0;
    }
    pthread_mutex_unlock(&pqu->tailMut);
    if(ele != 0){
        *ppdata = ele->data;
        free(ele);
        return ;
    }
	    
    ele = pqu->head;
    pqu->head = ele->next;
    *ppdata = ele->data;
    free(ele);
    return ;
}

/**
 *  * instant version.
 *   */
int mmq_take(MMQ_T *pqu, void **ppdata)
{
	 pthread_mutex_lock(&pqu->headMut);
	 if(pqu->head == 0){
	     pthread_mutex_unlock(&pqu->headMut);
	     return 0;
	 }
	 mmq_take_unlocked(pqu, ppdata);
	 pthread_mutex_unlock(&pqu->headMut);
	 return 1;
}

static void unlock_mutex(void *plock){
	    pthread_mutex_unlock((pthread_mutex_t *)plock);
}
/***
 *  * wait version 
 *   */
int mmq_takew(MMQ_T *pqu, void **ppdata)
{
	pthread_mutex_lock(&pqu->headMut);
	pthread_cleanup_push(unlock_mutex, &pqu->headMut);
	while(pqu->head == 0){
	   int retw = pthread_cond_wait(&pqu->headCnd, &pqu->headMut);
	   if(retw != 0){
	       fprintf(stderr, "error occurs in function pthread_cond_wait.\n");
	       pthread_mutex_unlock(&pqu->headMut);
	       pthread_exit((void*)1); 
	   }
	}
    mmq_take_unlocked(pqu, ppdata);
    pthread_cleanup_pop(1);
    return 1;
}

