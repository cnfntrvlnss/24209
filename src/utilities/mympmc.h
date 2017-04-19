/*************************************************************************
	> File Name: utilities/mympmc.h
	> Author: 
	> Mail: 
	> Created Time: Tue 22 Nov 2016 04:13:30 AM EST
 ************************************************************************/

#ifndef _UTILITIES/MYMPMC_H
#define _UTILITIES/MYMPMC_H

#include <pthread.h>
/****************
 * 多个线程为生产者，多个线程为消费者，队列容量不设限。
 *
 *
 */
typedef struct Element_T{
    void *data;
    struct Element_T *next;
}E_T;

typedef struct Queue_T{
    E_T *head;
    pthread_mutex_t headMut; // for head and its pointed struct.
    pthread_cond_t headCnd; // for head changing form null to not-null.
    E_T *tail;
    pthread_mutex_t tailMut;//for tail and its pointed struct.
    pthread_cond_t tailCnd;
}MMQ_T;

#define MMQ_T_INITCOPY  {0,PTHREAD_MUTEX_INITIALIZER, PTHREAD_COND_INITIALIZER, 0, PTHREAD_MUTEX_INITIALIZER, PTHREAD_COND_INITIALIZER}

/**
 * return 1 if success to retrieve data, or 0.
 */
int mmq_take(MMQ_T *pqu, void **ppdata);
int mmq_takew(MMQ_T *pqu, void **ppdata);

void mmq_put(MMQ_T *pqu, void *pdata);

unsigned mmq_size(MMQ_T *pqu);

#endif
