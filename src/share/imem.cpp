/*****************************************************************************/
/*M*
//                        THINKIT INTERNATIONAL PROPRIETARY INFORMATION
//        This software is supplied under the terms of the license agreement
//		or nondisclosure agreement with Thinkit International and may not be copied
//		or disclosed except in accordance with the terms of that agreement.
//            Copyright (c) 2002 ~ 2008 Thinkit International. All Rights Reserved.
//     VSS:
//     $Workfile:: imem.cpp                            $
//     $Author:: Jjwang                               $
//     $Revision:: 1                                  $
//     $Modtime:: 11/09/13 20:39                      $
//     $NoKeywords: $
//
//
//    Memory Management Modules
//
M*/

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#if defined __unix__
#include <unistd.h>
#include <sys/mman.h>
#else
#include <windows.h>
#endif
//#include <ipp.h>
//#include "comm.h"
#include "imem.h"

/**
ISDTAPI void *Malloc(int sz,const bool clear) throw(BadAlloc)
{
    void *pt=malloc(sz);
    if (!pt)
    {
        char str[50];
        sprintf (str, "Memory allocate %d bytes", sz);
        throw BadAlloc(str);
    }
    if (clear)
    {
        if (sz>1)
            ippsZero_16s ((short*)pt, sz>>1);
        if (sz&1) ((char*)pt)[sz-1]=(char)0;
    }
    return pt;
}

ISDTAPI void *Malloc(int n1,int itsz,const bool clear) throw(BadAlloc)
{
    return Malloc(n1*itsz,clear);
}

ISDTAPI void **Malloc(int n1,int n2,int itsz,const bool clear) throw(BadAlloc)
{
    void **tmp=(void **)Malloc(sizeof(void *)*n1);
    for (int i=0; i<n1; i++) tmp[i]=Malloc(itsz*n2,clear);
    return tmp;
}

ISDTAPI void ***Malloc(int n1,int n2,int n3,int itsz,const bool clear) throw(BadAlloc)
{
    void ***tmp=(void ***)Malloc(sizeof(void **)*n1);
    for (int i=0; i<n1; i++)
    {
        tmp[i]=(void **)Malloc(sizeof(void *)*n2);
        for (int j=0; j<n2; j++) tmp[i][j]=Malloc(itsz*n3,clear);
    }
    return tmp;
}


ISDTAPI void *Malloc32(int sz,const bool clear) throw(BadAlloc)
{
#ifdef WIN32
    char *ptr = (char*)Malloc(sz+32,clear);
    void *pt = (void*)(ptr + 32 - ((ptr - (char*)NULL) & 31));
#else
    char *ptr = (char*)Malloc(sz+64,clear);
    void *pt = (void*)(ptr + 64 - ((ptr - (char*)NULL) & 63));
#endif
    ((void**)pt)[-1]=ptr;
    return pt;
}
ISDTAPI void *Malloc32(int n1,int itsz,const bool clear) throw(BadAlloc)
{
    return Malloc32(n1*itsz,clear);
}

ISDTAPI void Free(void *ptr) throw()
{
    free(ptr);
}
ISDTAPI void Free32(void *ptr) throw()
{
    free(((void**)ptr)[-1]);
}

ISDTAPI void Free(void **ptr,int n1) throw()
{
    for (int i=0; i<n1; i++) Free(ptr[i]);
    Free(ptr);
}

ISDTAPI void Free(void ***ptr,int n1,int n2) throw()
{
    for (int i=0; i<n1; i++)
    {
        for (int j=0; j<n2; j++) Free(ptr[i][j]);
        Free(ptr[i]);
    }
    Free(ptr);
}

//**************** Class Memory **********************************

IHeap *Memory::CreateHeap(int sz)
{
    IHeap* pHeap = (IHeap*)Malloc(sizeof(IHeap));
    pHeap->size=sz;
    pHeap->head=NULL;
    pHeap->pHeap = NULL;
    return pHeap;
}

void Memory::DeleteHeap(IHeap *pHeap)
{
    aMemId *chunk=pHeap->pHeap;
    while (chunk)
    {
        aMemId *ptr=chunk;
        chunk=chunk->next;
        Free(ptr);
    }
    Free(pHeap);
    return;
}

void *Memory::New(IHeap *pHeap, const bool clear)
{
    aMemId *chunk=pHeap->pHeap;
    if (pHeap->head==NULL)
    {
        chunk=(aMemId *)Malloc(sizeof(aMemId),true);
        chunk->next=pHeap->pHeap;
        pHeap->pHeap=chunk;

        const int nelem=aMemId::size/pHeap->size;
        char* start=chunk->mem;
        char* last=&start[(nelem-1)*pHeap->size];
        for (char* p=start; p<last; p+=pHeap->size)
            reinterpret_cast<Link*>(p)->next=reinterpret_cast<Link*>(p+pHeap->size);
        reinterpret_cast<Link*>(last)->next=NULL;
        pHeap->head=reinterpret_cast<Link*>(start);
    }
    Link* ptr=pHeap->head;
    pHeap->head=ptr->next;
    if (clear)
        memset(ptr,0,pHeap->size);
    return ptr;
}

void *Memory::New(IHeap *pHeap, int len, const bool clear)
{
    aMemId *chunk=NULL;
    if (pHeap->head==NULL)
    {
        chunk=(aMemId *)Malloc(sizeof(aMemId),true);
        chunk->next=pHeap->pHeap;
        pHeap->pHeap=chunk;
        pHeap->head=reinterpret_cast<Link*>(chunk->mem);
    }
    char* start=reinterpret_cast<char*>(pHeap->head);
    char* last=pHeap->pHeap->mem+aMemId::size-sizeof(float);
    char* p=start;
    if (p+len>last)
    {
        chunk=(aMemId *)Malloc(sizeof(aMemId),true);
        chunk->next=pHeap->pHeap;
        pHeap->pHeap=chunk;
        char* start=chunk->mem;
        char* last=chunk->mem+aMemId::size-sizeof(float);
        char* p=start;
        reinterpret_cast<Link*>(p)->next=reinterpret_cast<Link*>(p+len);
        reinterpret_cast<Link*>(last)->next=NULL;
        pHeap->head=reinterpret_cast<Link*>(start);
    }
    else
    {
        reinterpret_cast<Link*>(p)->next=reinterpret_cast<Link*>(p+len);
        reinterpret_cast<Link*>(last)->next=NULL;
        pHeap->head=reinterpret_cast<Link*>(start);
    }

    Link* ptr=pHeap->head;
    pHeap->head=ptr->next;
    if (clear) memset(ptr,0,len);
    return ptr;
}

void Memory::Delete(IHeap *pHeap, void *ptr)
{
    Link* p=static_cast<Link*>(ptr);
    p->next=pHeap->head;
    pHeap->head=p;
}

*/