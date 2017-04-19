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

#include "TLI_API.h"

#define MAX_PATH 512
static void initTLI();
static void rlseTLI();
class LIDSpace4TLI{
    static LIDSpace4TLI* onlyone;
    static pthread_mutex_t onlyoneLock;

    LIDSpace4TLI(){
        initTLI();
    }
    ~LIDSpace4TLI(){
        rlseTLI();
    }
    friend void initLID(unsigned thrdNum);
};

unsigned g_nThreadNum = 8;
void initLID(unsigned thrdNum)
{
    pthread_mutex_lock(&LIDSpace4TLI::onlyoneLock);
    if(LIDSpace4TLI::onlyone == NULL){
        if(thrdNum != 0)g_nThreadNum = thrdNum;
        LIDSpace4TLI::onlyone = new LIDSpace4TLI();
        if(LIDSpace4TLI::onlyone == NULL){
            fprintf(stderr, "ERROR in LIDSpace4TLI::initLID, failed to create new instance of LIDSpace4TLI.\n");
        }
    }
    pthread_mutex_unlock(&LIDSpace4TLI::onlyoneLock);
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
        exit(1);
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
void initTLI()
{
    g_nTemplateNum = countLinesInFile("ioacas/sysdir/param.txt");
    for(int idx=0; idx < g_nTemplateNum; idx ++){
        g_pnAllTemplateIDs[idx] = idx;
        char *curname = new char[10];
        snprintf(curname, 10, "%d", idx);
        g_pszAllTemplateNames[idx] = curname;
    }
    
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
        exit(1);
    }
}

void rlseTLI()
{
    TLI_Exit();  
}

int openTLI_dup()
{
    initLID(0);
    TLI_HANDLE hret = -1;
    int err = TLI_Open(hret);
    if(hret < 0){
        fprintf(stderr, "ERROR in openTLI_dup, fail to call TLI_Open_1. error: %d.\n", err);
    }

    return hret;
}

void closeTLI_dup(int hdl)
{
    TLI_Close(hdl);
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
