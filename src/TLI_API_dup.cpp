/*************************************************************************
    > File Name: TLI_API_dup.cpp
    > Author: zhengshurui
    > Mail:  zhengshurui@thinkit.cn
    > Created Time: Sat 08 Oct 2016 03:19:25 AM PDT
 ************************************************************************/

#include <pthread.h>
#include <cctype>
#include <cstdio>
#include <cassert>
#include <cstdlib>

#define DLOPENSO
#ifndef DLOPENSO
#include "TLI_API.h"
#else
#include <dlfcn.h>
static void *g_LidHdl;

typedef int TLI_HANDLE;
typedef int (*TLI_Init_P)(            
							char * pszTLISystemDirectory,
							int * pnAllTemplateIDs,
							char ** ppszAllTemplateLocation,
							int nAllTemplateNumber,
							int nLineNumber
						);
typedef int (*TLI_Exit_P)();
typedef int (*TLI_Open_P)(
	TLI_HANDLE & hTLI
);
typedef int (*TLI_Close_P)(
	TLI_HANDLE hTLI
);
typedef int (*TLI_Recognize_P)(
	TLI_HANDLE hTLI,
	int * pnCurrentTemplateIDs,
	int nCurrentTemplateNumber,
	void * pvPCMBuffer,
	int nBytesOfBuffer,
	int nMinLimit,
	int nMaxLimit
);
typedef int (*TLI_GetResult_P)(
	TLI_HANDLE hTLI,
	float * pfResults,
	int nNumber
);
static TLI_Init_P TLI_Init;
static TLI_Exit_P TLI_Exit;
static TLI_Open_P TLI_Open;
static TLI_Close_P TLI_Close;
static TLI_Recognize_P TLI_Recognize;
static TLI_GetResult_P TLI_GetResult;

#define GETSMPOL(x, y) \
        x = (x##_P)dlsym(g_LidHdl, y);\
        do{\
           if(x == NULL){fprintf(stderr, "ERROR %s", dlerror()); } \
        } while(0)


void openlideng()
{
    const char *lidfile = "ioacas/libTLI_API.so";
    g_LidHdl = dlopen(lidfile, RTLD_LAZY);
    if(g_LidHdl == NULL){
        fprintf(stderr, "ERROR %s", dlerror());
    }
    GETSMPOL(TLI_Init, "_Z8TLI_InitPcPiPS_ii");
    GETSMPOL(TLI_Exit, "_Z8TLI_Exitv");
    GETSMPOL(TLI_Open, "_Z8TLI_OpenRi");
    GETSMPOL(TLI_Close, "_Z9TLI_Closei");
    GETSMPOL(TLI_GetResult, "_Z13TLI_GetResultiPfi");
    GETSMPOL(TLI_Recognize, "_Z13TLI_RecognizeiPiiPviii");
}

#endif

#define MAX_PATH 512
static bool initTLI();
static void rlseTLI();
class LIDSpace4TLI{
    static LIDSpace4TLI* onlyone;
    static pthread_mutex_t onlyoneLock;

    LIDSpace4TLI(){
        bInit = initTLI();
    }
    ~LIDSpace4TLI(){
        rlseTLI();
    }
    friend bool initLID(unsigned thrdNum);
	bool bInit;
};

unsigned g_nThreadNum = 8;
bool initLID(unsigned thrdNum)
{
	bool ret = false;
    pthread_mutex_lock(&LIDSpace4TLI::onlyoneLock);
    if(LIDSpace4TLI::onlyone == NULL){
        if(thrdNum != 0)g_nThreadNum = thrdNum;
        LIDSpace4TLI::onlyone = new LIDSpace4TLI();
        if(LIDSpace4TLI::onlyone == NULL){
            fprintf(stderr, "ERROR in LIDSpace4TLI::initLID, failed to create new instance of LIDSpace4TLI.\n");
        }
    }
	ret = LIDSpace4TLI::onlyone->bInit;
    pthread_mutex_unlock(&LIDSpace4TLI::onlyoneLock);
	return ret;
}

LIDSpace4TLI* LIDSpace4TLI::onlyone = NULL;
pthread_mutex_t LIDSpace4TLI::onlyoneLock = PTHREAD_MUTEX_INITIALIZER;

#define LANGNUM_MAX 100
char g_SysDirPath[MAX_PATH] = "ioacas/sysdir";
int g_nTemplateNum=19;
int g_pnAllTemplateIDs[LANGNUM_MAX];
char *g_pszAllTemplateNames[LANGNUM_MAX];
//int *g_pnTemplateIDs = NULL;
int g_SecondVAD=0;
int g_bUseDetector=0;
int g_MaxLIDLen = 3600;
int g_MinLIDLen = 5;
const unsigned MAXLINE_LEN = 100;

unsigned countLinesInFile(const char *file)
{
    unsigned cnt = 0;   
    FILE *fp = fopen(file, "r");
    if(fp == NULL){
        fprintf(stderr, "cannot open file %s.\n", file);
		return 0;
    }
    char szTemp[MAXLINE_LEN];
    while(!feof(fp)){
        char *stPtr = fgets(szTemp, MAXLINE_LEN, fp);
        if(stPtr == NULL) break;
        while(*stPtr != '\0'){
            if(isalnum(*stPtr))break;
            stPtr ++;
        }
        if(*stPtr != '\0') cnt ++;
    }
    return cnt;
}
bool initTLI()
{
	bool ret = false;
    g_nTemplateNum = countLinesInFile("ioacas/sysdir/param.txt");
	if(g_nTemplateNum == 0) return ret;
    for(int idx=0; idx < g_nTemplateNum; idx ++){
        g_pnAllTemplateIDs[idx] = idx;
        char *curname = new char[10];
        snprintf(curname, 10, "%d", idx);
        g_pszAllTemplateNames[idx] = curname;
    }
    
    openlideng();
	if(!TLI_Init) return ret;
    int tliret = TLI_Init(g_SysDirPath, g_pnAllTemplateIDs, g_pszAllTemplateNames, g_nTemplateNum, g_nThreadNum);
    if(tliret != 0){
        if(tliret == -1){
            fprintf(stderr, "ERROR model path is incorrect.\n");
        }
        else if(tliret == -2){
            fprintf(stderr, "ERROR license is incorrect.\n");
        }
        else if(tliret == -3){
            fprintf(stderr, "ERROR setting of thread number is incorrect.\n");
        }
        else if(tliret == -4){
            fprintf(stderr, "ERROR model file is not found.\n");
        }
        else{
            fprintf(stderr, "ERROR other error in tli_init.\n");
        }
    }
	else{ ret = true; }
	return ret;
}

void rlseTLI()
{
	if(TLI_Exit) TLI_Exit();  
}

int openTLI_dup()
{
    initLID(0);
    TLI_HANDLE hret = -1;
	if(TLI_Open == NULL) return hret;
    int err = TLI_Open(hret);
    if(hret < 0){
        fprintf(stderr, "ERROR in openTLI_dup, fail to call TLI_Open_1. error: %d.\n", err);
    }

    return hret;
}

void closeTLI_dup(int hdl)
{
	if(TLI_Close) TLI_Close(hdl);
}

void scoreTLI_dup(int hdl, short *pcmData, int pcmLen, int &resID, float &resScore)
{
    int nMaxSec = 3600;
    int nMinSec = 5;
    resID = -1;
    resScore = 0.0;
    int err = TLI_Recognize(hdl, g_pnAllTemplateIDs, g_nTemplateNum, reinterpret_cast<char*>(pcmData), pcmLen * sizeof(short), nMinSec, nMaxSec);
    if(err != 0){
        if(err == -1){
            fprintf(stderr, "ERROR in tli_recognize, wrong handle.\n");
        }
        else if(err == -2){
            fprintf(stderr, "ERROR in tli_recognize, the current thread is not initialized.\n");
        }
        else if(err == -3){
            fprintf(stderr, "ERROR in tli_recognize, the index array is empty, or the lang num is not great than zero.\n");
        }
        else if(err == -4){
            fprintf(stderr, "ERROR in tli_recognize, system go wrong.\n");
        }
        else if(err == -5){
            //the audio length is too short.
        }
        else if(err == -6){
            //the length after vad or music is too short.
        }
        else {
            fprintf(stderr, "ERROR in tli_recognize, other error.\n");
        }
        return ;
    }
    
    float scoreArr[LANGNUM_MAX];
    int langNum = g_nTemplateNum + 1;
    for(int idx=0; idx < langNum; idx++){
        scoreArr[idx] = 0.0;
    }
    err = TLI_GetResult(hdl, scoreArr, langNum);
    if(err == -1){
        fprintf(stderr, "ERROR in TLI_GetResult.");
        return ;
    }
    for(int idx=0; idx < g_nTemplateNum; idx++){
        if(resScore < scoreArr[idx]){
            resScore = scoreArr[idx];
            resID = idx;
        }
    }
}
