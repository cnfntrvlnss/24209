/*************************************************************************
	> File Name: sni_kw_test.cpp
	> Author: 
	> Mail: 
	> Created Time: Thu 04 May 2017 10:35:35 PM EDT
 ************************************************************************/

#include "sni_kw_ex.h"
#include "wav/wav.h"

#include <cstdlib>
#include <cstdio>
#include <cstring>

#include <string>
#include<iostream>
using namespace std;

static bool g_bDebugMode = true;
#define DEBUG_LOG(FMT, ...) if(g_bDebugMode) fprintf(stdout, FMT "\n", ##__VA_ARGS__);
#define ERROR_LOG(FMT, ...) fprintf(stderr, "ERROR " FMT "\n", ##__VA_ARGS__);
static bool loadAudio(const char *curfile, short *&wavbuf, int &wavlen)
{
    short *&wavbuf1= wavbuf;
    int &wavlen1 = wavlen;
    short *wavbuf2 = NULL;
    int wavlen2 = 0;
    int nchl;
    if(!CreateWav(curfile, nchl, wavbuf1, wavlen1, wavbuf2, wavlen2)){
        ERROR_LOG("failed to load wave from file: %s.", curfile);
        return false;
    }
    DestroyWav(wavbuf2, wavlen2);
    return true;
}

static inline void str_rstrip(char *line)
{
    char *rptr = strrchr(line, '\0');
    while(--rptr >= line && strchr("\n\r \t", *rptr) != NULL) *rptr = '\0';
}

#define MAX_PATH 512
static string g_ListFile= "audiolist";
static unsigned g_ThreadNum = 1;
void sni_kw_process()
{
    int hdl;
    if(!sni_open(hdl)){
        exit(1);
    }
    short *wavbuf;
    int wavlen;
    FILE *fp = fopen(g_ListFile.c_str(), "r");
    if(fp == NULL){
        ERROR_LOG("failed to open listfile %s", g_ListFile.c_str());
        exit(1);
    }
    char tmpline[MAX_PATH];
    while(fgets(tmpline, MAX_PATH, fp) != NULL){
        str_rstrip(tmpline);
        if(tmpline[0] == '\0') continue;
        if(!loadAudio(tmpline, wavbuf, wavlen)){
            continue;
        }
        int score;
        if(isAudioSynthetic(hdl, wavbuf, wavlen, score)){
            DEBUG_LOG("synthetic speech found, %s %d", tmpline, score);
        }

        DestroyWav(wavbuf, wavlen);
    }
    fclose(fp);
    sni_close(hdl);
}

int main(int argc, char *argv[])
{
    sni_init(g_ThreadNum);
    sni_kw_process();
    return 0;
}

