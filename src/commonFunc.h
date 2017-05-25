/*
 *=================================================================
 *      Copyright (c) 2015 By ThinkIT Technologies Co Ltd
 *                       All Rights Reserved.
 *
 * FNAME: commonFunc.h
 * DESCP: <+Brief Descripation+>
 * AUTHR: Xiao Benfang,xiaobenfang@thinkit.cn
 * CREAT: 2015-10-08 17:34:14
 * WSITE: http://www.thinkit.com.cn
 * LC_AT: 2015-10-08 17:34:14
 * VERNO: 1.0.0
 * CNOTE: <+Latest Change+>
 *=================================================================
 */
# ifndef _COMMONFUNC_H
# define _COMMONFUNC_H

#include <cstdarg>
#include <cstdio>
#include <string>
#include <vector>
#include <ctype.h>
#include <iostream>
#include <fstream>
#include <map>
#include <vector>
#include <algorithm>
#include <list>

#include <sys/ioctl.h>  
#include <net/if.h>  
#include <arpa/inet.h>  
#include <sys/stat.h>
#include <stdarg.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/time.h>
#include <unistd.h>

#include <cassert>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <cerrno>
#include <string>
#include "../include/interface242.h"
#include "wav/waveinfo.h"
#include "utilites.h"
#include "socket_ex.h"
#include "log4z.h"
extern zsummer::log4z::ILog4zManager *g_Log4zManager;
extern LoggerId g_logger;
extern ConfigRoom g_AutoCfg;

#define MAX_PATH 512
extern char g_szDebugBinaryDir[MAX_PATH];

#define PCM_ONESEC_LEN 16000
#define PCM_ONESEC_SMPS 8000
#define POSITIVE_PCM_LEN (PCM_ONESEC_LEN * 10)

#define CHECK_PERFOMANCE
#ifdef CHECK_PERFOMANCE

void clockoutput_start(const char *fmt, ...);
std::string clockoutput_end();
#else
static void clockoutput_start(const char *fmt, ...){}
static std::string clockoutput_end(){return "";}
#endif 

bool if_directory_exists(const char *dir, bool bForce = false);
int save_binary_data(const char *filePath, const void* ptr, size_t num, ...);

bool  gen_spk_save_file(char *savedname, const char *topDir, const char *subDir, time_t curtime, unsigned long id, const unsigned short *typeId = NULL, const unsigned *userId = NULL, const int *confidence = NULL);
bool saveWave(char *pData, unsigned len, const char *saveFileName);

std::string saveTempBinaryData(struct timeval curtime, unsigned long pid, char* data, unsigned len);
std::string saveProjectSegment(struct timeval curtime, unsigned long pid, unsigned *offset, char* data, unsigned len);
# endif  
