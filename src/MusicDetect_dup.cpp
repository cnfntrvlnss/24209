/*************************************************************************
    > File Name: MusicDetect_dup.cpp
    > Author: zhengshurui
    > Mail:  zhengshurui@thinkit.cn
    > Created Time: Fri 07 Oct 2016 07:21:42 PM PDT
 ************************************************************************/

#include <cstdio>
#include <cassert>
#include <cstdlib>
#include <cstring>

#include <pthread.h>
#include <set>

#include "MusicDetect.h"
#include "MusicDetect_dup.h"

using namespace std;
#define MAX_PATH 512


static const unsigned char MAX_MUSICCUT = 10;
static MscCutInst g_arrAllMscCuts[MAX_MUSICCUT];
static unsigned g_uAccumMscCuts = 0;
static char g_szCfgFile[MAX_PATH] = {0};
static set<unsigned> g_setOpenedInstIndice;
static bool g_bMscCutsInited = false;
static pthread_mutex_t g_MscCutsLock = PTHREAD_MUTEX_INITIALIZER;

MscCutHandle openMusicCut(const char *cfgFile)
{
    pthread_mutex_lock(&g_MscCutsLock);
    if(g_uAccumMscCuts == 0){
        strncpy(g_szCfgFile, cfgFile, MAX_PATH);
    }
    else{
        if(cfgFile != NULL && strcmp(g_szCfgFile, cfgFile) != 0){
            fprintf(stderr, "WARN in openMusicCut, the parameter of cfgFile is not consistent of the previous ones.\n");
        }
    }
    MscCutHandle ret = NULL;
    if(g_uAccumMscCuts < MAX_MUSICCUT){
        unsigned curIdx = g_uAccumMscCuts ++;
        ret = &g_arrAllMscCuts[curIdx];
        g_setOpenedInstIndice.insert(curIdx);
    }   
    else{
        fprintf(stderr, "ERROR in openMusicCut, the total MusicCuts already accumulate to MAX_MUSICCUT.\n");
    }
    pthread_mutex_unlock(&g_MscCutsLock);
    return ret;
}

void finishOpenMusicCut()
{
    pthread_mutex_lock(&g_MscCutsLock);
    if(!g_bMscCutsInited){
        assert(g_uAccumMscCuts > 0);
        assert(strcmp(g_szCfgFile, "") != 0);
        if(!MusicCut_Initial(g_szCfgFile, g_uAccumMscCuts)){
            fprintf(stderr, "FATAL in finishOpenMusicCut, failed initialize MusicCut engine. cfgFile: %s; threadNum: %u.\n", g_szCfgFile, g_uAccumMscCuts);
            exit(1);
        }
        g_bMscCutsInited = true;
    }
    pthread_mutex_unlock(&g_MscCutsLock);
}

bool cutMusic(MscCutHandle hdl, short* src, unsigned iLen, short* &dst, unsigned &oLen)
{
    assert(hdl - g_arrAllMscCuts < g_uAccumMscCuts);
    int olen = iLen;
    MusicCut(hdl - g_arrAllMscCuts, src, iLen, dst, olen);
    oLen = olen;
    return true;
}

void closeMusicCut(MscCutHandle hdl)
{
    pthread_mutex_lock(&g_MscCutsLock);
    unsigned curIdx = hdl - g_arrAllMscCuts;
    if(g_setOpenedInstIndice.find(curIdx) == g_setOpenedInstIndice.end()){
        fprintf(stderr, "ERROR in closeMusicCut, the handle passed in is not opened.\n");
    }
    else{
        g_setOpenedInstIndice.erase(curIdx);
    }
    if(g_setOpenedInstIndice.empty()){
        MusicCut_Free();
    }
    pthread_mutex_unlock(&g_MscCutsLock);
}
