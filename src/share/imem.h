/*****************************************************************************/
/*M*
//                        THINKIT INTERNATIONAL PROPRIETARY INFORMATION
//        This software is supplied under the terms of the license agreement
//		or nondisclosure agreement with Thinkit International and may not be copied
//		or disclosed except in accordance with the terms of that agreement.
//            Copyright (c) 2002 ~ 2008 Thinkit International. All Rights Reserved.
//     VSS:
//     $Workfile:: imem.h                              $
//     $Author:: Jjwang                               $
//     $Revision:: 1                                  $
//     $Modtime:: 11/09/13 20:39                      $
//     $NoKeywords: $
//
//
//    Memory Management Head File
//
M*/

#ifndef aaaiMEMORY_MANAGEMENTaaa
#define aaaiMEMORY_MANAGEMENTaaa
#include <malloc.h>
#include "mem.h"
#include "isdtexception.h"

static const uint hSize=100*1024;

struct Link
{
    Link* next;
};

struct aMemId
{
    enum {size=hSize};
    aMemId *next;
    char mem[size];
};

struct IHeap
{
    int size;            /* size */
    Link* head;
    aMemId *pHeap;
};
/**
class ISDTAPI Memory
{
public:
    static IHeap *CreateHeap(int sz=0);
    static void DeleteHeap(IHeap *id);
    static void *New(IHeap *id, int len, const bool clear=false);
    static void *New(IHeap *id, const bool clear=false);
    static void Delete(IHeap *id, void *ptr);
};
*/
#endif
