#ifndef __PORTING_MODULE__WIN
#define __PORTING_MODULE__WIN

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <math.h>
#include <assert.h>
#include <stdarg.h>


#ifdef WIN32
#include <conio.h>
#include <windows.h>
#include <process.h>
#include <direct.h>
#include <io.h>
#include <time.h>
#endif


#include <iostream> // zjp: following 3 lines .h removed
#include <iomanip>
#include <fstream>

#include <string>
#include <vector>
#include <queue>
#include <map>


/***
#ifdef WIN32
#include <hash_map>
#include <hash_set>
#include <algorithm>
#else
#include <ext/hash_map>
#include <ext/hash_set>
#include <ext/hash_fun.h>
#endif
*****/
/****
#if defined __GNUC__
using __gnu_cxx::hash_map;
using __gnu_cxx::hash;
#endif
*****/
using namespace std;

//////////////////////////////////////////////////////////////////////////
//--- type definition
#ifdef WIN32
typedef _int64	INT64;
typedef unsigned long DWORD;
#else
typedef long long INT64;
#ifndef __INTEL_COMPILER
typedef long long __int64;
#endif
typedef unsigned long DWORD;
#define  HANDLE int
#endif

typedef unsigned short	ushort;
typedef unsigned int	uint;
typedef unsigned char	uchar;
typedef ushort	WordID;

/********
//string hashmap
struct equal_str
{
    bool operator()(const string& s1, const string& s2) const
    {
        return s1 == s2;
    }
};
struct hash_int64
{
    size_t operator()(const __int64 in)  const
    {
#ifdef WIN32 // zjp: 2012-11-02 add WIN32 part
        __int64 ret = (in >> 32L) ^ (in & 0xFFFFFFFF);
#else
        long long ret = (in >> 32L) ^ (in & 0xFFFFFFFF);
#endif
        return (size_t) ret;
    }
};
struct equal_int64
{
    bool operator()(const __int64 s1, const __int64 s2) const
    {
        return s1 == s2;
    }
};
#if defined __GNUC__
struct str_hash
{
    size_t operator()(const string& str) const
    {
        hash<const char*> hchar;
        return hchar(str.c_str());
         //unsigned long __h = 0;
         //for (size_t i = 0 ; i < str.size() ; i ++)
         //__h = 5*__h + str[i];
         return size_t(__h);
    }
};
typedef hash_map<string, WordID, str_hash, equal_str> WordIntHMap;
typedef hash_map<string, string, str_hash, equal_str> WordWordHMap;
#else
typedef hash_map<string, WordID, hash<string>, equal_str> WordIntHMap;
typedef hash_map<string, string, hash<string>, equal_str> WordWordHMap;

#endif
typedef hash_map<int,__int64,hash_int64> int64IDMap;
*****/
//Multi-thread
#ifndef WIN32
#include <pthread.h>
#include <sys/resource.h>
#include <sys/time.h>
#include <unistd.h>
#include <sys/timeb.h>
#include <sys/types.h>
#include <sys/errno.h>
#include <sys/stat.h>
#define CRITICAL_SECTION pthread_mutex_t
//void InitializeCriticalSection(CRITICAL_SECTION*x);
#define EnterCriticalSection pthread_mutex_lock
#define LeaveCriticalSection pthread_mutex_unlock
#define DeleteCriticalSection pthread_mutex_destroy
#define MAX_PATH 512
#define Sleep(n) usleep(n*1000)
#undef __stdcall
#endif

#endif /// __PORTING_MODULE__
