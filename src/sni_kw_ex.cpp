/*************************************************************************
	> File Name: sni_kw_ex.cpp
	> Author: 
	> Mail: 
	> Created Time: Thu 04 May 2017 10:12:49 PM EDT
 ************************************************************************/

#include "SNI_API.h"
#include <cstdlib>
#include <cstdio>

#include<iostream>
using namespace std;

# ifndef _COMMONFUNC_H
enum BLOG_LEVEL{BLOGT, BLOGD, BLOGI, BLOGW, BLOGE};
#define BLOGLMT BLOGI
#define BIFO(x) if(BLOGLMT >= BLOG##x) 
#define BLOGE(fmt, ...)  BIFO(E) fprintf(stderr, "ERROR "fmt "\n", ##__VA_ARGS__)
#define BLOGW(fmt, ...) BIFO(W) fprintf(stderr, "WARN "fmt"\n", ##__VA_ARGS__)
#define BLOGI(fmt, ...) BIFO(I) fprintf(stderr, "INFO "fmt"\n", ##__VA_ARGS__)
#define BLOGD(fmt, ...) BIFO(D) fprintf(stderr, "DEBUG "fmt"\n", ##__VA_ARGS__)
#define BLOGT(fmt, ...) BIFO(T) fprintf(stderr, "TRACE "fmt"\n", ##__VA_ARGS__)
#else
#define BLOGE(fmt, ...) LOGFMT_ERROR(g_logger, fmt, ##__VA_ARGS__)
#define BLOGW(fmt, ...) LOGFMT_WARN(g_logger, fmt, ##__VA_ARGS__)
#define BLOGI(fmt, ...) LOGFMT_INFO(g_logger, fmt, ##__VA_ARGS__)
#define BLOGD(fmt, ...) LOGFMT_DEBUG(g_logger, fmt, ##__VA_ARGS__)
#define BLOGT(fmt, ...) LOGFMT_TRACE(g_logger, fmt, ##__VA_ARGS__)
#endif

void sni_exit()
{
    SNI_Exit();
}

void sni_init(unsigned thrdnum)
{
    const char * snisysdir = "ioacas/SNISysdir";
    int err = SNI_Init(const_cast<char*>(snisysdir), static_cast<int>(thrdnum));
    if(err !=0){
        BLOGE("failed to call SNI_Init, error: %d.", err);
        exit(1);
    }
    atexit(sni_exit);
}

bool sni_open(int &hdl)
{
    if(SNI_Open(hdl) != 0){
        BLOGE("failed to call SNI_Open");
        return false;
    }
    return true;
}

bool sni_close(int hdl)
{
    if(SNI_Close(hdl) != 0){
        BLOGE("failed to call SNI_Close");
        return false;
    }
    return true;
}

bool isAudioSynthetic(int hdl, short *pcmdata, unsigned pcmlen, int &maxscore)
{
    float scores[3];
    int err = SNI_Recognize(hdl, pcmdata, pcmlen, -1, -1);
    if(err != 0){
        BLOGE("failed to call SNI_Recognize!");
        return false;
    }
    err = SNI_GetResult(hdl, scores, 3);
    if(err != 0){
        BLOGE("failed to call SNI_GetResult.");
    }
    float score = scores[0];
    unsigned maxidx = 0;
    for(unsigned idx=1; idx < 3; idx ++){
        if(score < scores[idx]){
            score = scores[idx];
            maxidx = idx;
        }
    }
    if(maxidx == 1){
        maxscore = score * 100;
        return true;
    }
    else{
        return false;
    }
}


