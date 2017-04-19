/*************************************************************************
    > File Name: dllVAD_dup.cpp
    > Author: zhengshurui
    > Mail:  zhengshurui@thinkit.cn
    > Created Time: Fri 07 Oct 2016 06:33:13 PM PDT
 ************************************************************************/
#include "utilites.h"
#include "dllVAD_dup.h"

#include <vector>
//#include<list>
#include <pthread.h>
#include <cstdio>
#include <sstream>

using namespace std;

# ifndef _COMMONFUNC_H
enum BLOG_LEVEL{BLOG_TRACE, BLOG_DEBUG, BLOG_INFO, BLOG_WARN, BLOG_ERROR};
#define BLOGLMT BLOG_INFO
#define BIFO(x, fmt, ...) if(BLOGLMT >= BLOG_##x) fprintf(stderr, #x" " fmt "\n", ##__VA_ARGS__)
#define BLOGE(fmt, ...) BIFO(ERROR, fmt, ##__VA_ARGS__)
#define BLOGW(fmt, ...) BIFO(WARN, fmt, ##__VA_ARGS__)
#define BLOGI(fmt, ...) BIFO(INFO, fmt, ##__VA_ARGS__)
#define BLOGD(fmt, ...) BIFO(DEBUG, fmt, ##__VA_ARGS__)
#define BLOGT(fmt, ...) BIFO(TRACE, fmt, ##__VA_ARGS__)
#else
#define BLOGE(fmt, ...) LOGFMT_ERROR(g_logger, fmt, ##__VA_ARGS__)
#define BLOGW(fmt, ...) LOGFMT_WARN(g_logger, fmt, ##__VA_ARGS__)
#define BLOGI(fmt, ...) LOGFMT_INFO(g_logger, fmt, ##__VA_ARGS__)
#define BLOGD(fmt, ...) LOGFMT_DEBUG(g_logger, fmt, ##__VA_ARGS__)
#define BLOGT(fmt, ...) LOGFMT_TRACE(g_logger, fmt, ##__VA_ARGS__)
#endif

static VADCfg g_cfgParam;
//static list<VADHandle> g_AllHandles;
static vector<bool> g_AllHandles;
static pthread_mutex_t g_AllHandlesLock = PTHREAD_MUTEX_INITIALIZER;
//static unsigned long g_ulAccumNum = 1;

static void initVAD(const char* cfgfile)
{
    ConfigRoom cfg(cfgfile);   
    Config_getValue(&cfg, "VAD", "ThreadNum", g_cfgParam.iThreadNum);
    Config_getValue(&cfg, "VAD", "OutWavSec", g_cfgParam.iOutWavSec);
    Config_getValue(&cfg, "VAD", "MinProcSec", g_cfgParam.iMinProcSec);
    Config_getValue(&cfg, "VAD", "MaxProcSec", g_cfgParam.iMaxProcSec);
    Config_getValue(&cfg, "VAD", "MaxDetectSec", g_cfgParam.iMaxDetectSec);
    Config_getValue(&cfg, "VAD", "DetectRing", g_cfgParam.bDetectRing);
    Config_getValue(&cfg, "VAD", "DetectSong", g_cfgParam.bDetectSong);
    Config_getValue(&cfg, "VAD", "DetectMusic", g_cfgParam.bDetectMusic);
    ostringstream oss;
    oss << "+++++++++++++++++LID VAD Config+++++++++++++++++";
    oss<< "ThreadNum="<< g_cfgParam.iThreadNum<< endl;
    oss<< "OutWavSec="<< g_cfgParam.iOutWavSec<< endl;
    oss<< "MinProcSec="<< g_cfgParam.iMinProcSec<< endl;
    oss<< "MaxProcSec="<< g_cfgParam.iMaxProcSec<< endl;
    oss<< "MaxDetectSec="<< g_cfgParam.iMaxDetectSec<< endl;
    oss<< "DetectRing="<< g_cfgParam.bDetectRing<< endl;
    oss<< "DetectSong="<< g_cfgParam.bDetectSong<< endl;
    oss<< "DetectMusic="<< g_cfgParam.bDetectMusic<< endl;
    oss<< endl;
    BLOGI("%s", oss.str().c_str());
    g_AllHandles.resize(g_cfgParam.iThreadNum);
    for(int idx=0; idx < g_cfgParam.iThreadNum; idx++){
        g_AllHandles[idx] = false;
    }
}

int openOneVAD(char *cfgFile)
{
    pthread_mutex_lock(&g_AllHandlesLock);
    int ret = 0;
    for(; ret < g_AllHandles.size(); ret ++){
        if(g_AllHandles[ret] == false) break;
    }
    if(ret == 0){
        bool bOpened = false;
        for(unsigned idx=1; idx < g_AllHandles.size(); idx ++){
            if(g_AllHandles[idx]) bOpened = true;
        }
        if(!bOpened){
            if(InitializeVADs(g_cfgParam)){
                BLOGI("%s", "InitializeVADs success.");
            }
            else{
                BLOGE("%s", "InitializeVADs failed.");
                ret = -1;
            }
        }
    }

    pthread_mutex_unlock(&g_AllHandlesLock);
    return ret;
}

void closeOneVAD(int hdl)
{
    pthread_mutex_lock(&g_AllHandlesLock);
    if(hdl < 0 || hdl > g_AllHandles.size()){
        BLOGE("invalid handle of VAD. hdl: %d.", hdl);
    }
    if(!g_AllHandles[hdl]){
        BLOGW("handle of VAD is already closed. hdl: %d.", hdl);
    }
    g_AllHandles[hdl] = false;
    int ret = 0;
    for(; ret < g_AllHandles.size(); ret ++){
        if(g_AllHandles[ret] == true) break;
    }
    if(ret == g_AllHandles.size()){
    //if(g_AllHandles.empty()){
        FreeVADs();
    }
    pthread_mutex_unlock(&g_AllHandlesLock);

}

