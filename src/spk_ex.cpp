/*************************************************************************
    > File Name: spk_ex.cpp
    > Author: zhengshurui
    > Mail:  zhengshurui@thinkit.cn
    > Created Time: Sun 09 Oct 2016 07:27:23 PM PDT
 ************************************************************************/

#include "spk_ex.h"
#include <stdlib.h>

#include <cassert>
#include <cstdio>

#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <list>
#include <map>
#include <set>

#define DLOPENSO
#ifndef DLOPENSO
#include "interface.h"
#else
#include <dlfcn.h>
struct WAVE 
{
	short* wavbuf;
	int wavlen;
	WAVE()
	{
		wavbuf = NULL;
		wavlen = -1;
	}
};
typedef enum
{
	StsLicenseExpired = -2,			// 授权过期
	StsNoLicense = -1,				// 没有授权
	StsNoError = 0,					// 成功
	StsNotInit = 1,					// 引擎没有初始化
	StsInitAgain = 2,				// 引擎多次初始化
	StsNoCfgFile = 3,				// 无法打开配置文件
	StsErrCfgItem = 4,				// 配置错误
	StsErrModel = 5,				// 与模型类相关的错误
	StsInvalidInput = 6,			// 无效输入
	StsWavTooShort = 7,				// 语音数据太短
	StsErrAudioSeg = 8,				// 与VAD相关的错误
	StsErrCluster = 9,				// 说话人分段聚类错误
	StsErrExFeat = 10,				// 提取特征失败
	StsNoMemory = 11,				// 分配内存失败
	StsErrOpenFile = 12,			// 无法打开文件
	StsErrWriteFile = 13,			// 写文件失败
	StsErrReadFile = 14,			// 读文件失败
} TITStatus;

void *g_SpkHdl;
typedef TITStatus (*spkinitfunc)(const char *);
static spkinitfunc TIT_SPKID_INIT;
typedef TITStatus (*spkexitfunc)();
static spkexitfunc TIT_SPKID_EXIT;
typedef TITStatus (*spkdelmfunc)(void *&mdl);
static spkdelmfunc TIT_SPKID_DEL_MDL;
typedef TITStatus (*spkidfunc)(const short * wavbuf,const int wavlen,const void** spks,const int numspk,float* scores);
static spkidfunc TIT_SPKID_VERIFY_CLUSTER;
typedef TITStatus (*spkldmfunc)(void *&mdl,const char *const modelpath);
static spkldmfunc TIT_SPKID_LOAD_MDL_IVEC;
//typedef TITStatus (*spktrnmfunc)(const WAVE * wavs,const int n,void *&Ivec);
//spktrnmfunc TIT_SPKID_TRN_SPK_MDL_IVEC;
typedef TITStatus (*spksavmfunc)(const void *const Ivec,const char *const modelpath);
static spksavmfunc TIT_SPKID_SAVE_MDL_IVEC;

#define GETSMPOL(x, type) \
        x = (type)dlsym(g_SpkHdl, #x);\
        do{\
           if(x == NULL){fprintf(stderr, "ERROR %s", dlerror()); exit(1);} \
        } while(0)
static void openspkeng()
{
    const char *spkfile = "ioacas/libSpeakerRecogAPI.so";
    g_SpkHdl = dlopen(spkfile, RTLD_LAZY);
    if(g_SpkHdl == NULL){
        fprintf(stderr, "ERROR %s", dlerror());
        exit(1);
    }
    GETSMPOL(TIT_SPKID_INIT, spkinitfunc);
    GETSMPOL(TIT_SPKID_EXIT, spkexitfunc);
    GETSMPOL(TIT_SPKID_DEL_MDL, spkdelmfunc);
    GETSMPOL(TIT_SPKID_VERIFY_CLUSTER, spkidfunc);
    GETSMPOL(TIT_SPKID_LOAD_MDL_IVEC, spkldmfunc);
    GETSMPOL(TIT_SPKID_SAVE_MDL_IVEC, spksavmfunc);

}

#endif

using namespace std;
#define SLOGE_FMT(fmt, ...) fprintf(stderr, "ERROR " fmt "\n", ##__VA_ARGS__)
#define SLOGW_FMT(fmt, ...) fprintf(stderr, "WARN " fmt "\n", ##__VA_ARGS__)

bool SpkInfo::fromStr(const char* param)
{
    istringstream iss(param);
    iss >> spkId;
    return true;
}
string SpkInfo::toStr() const
{
    ostringstream oss;
    oss<< spkId << ".param";
    return oss.str();
}

/*
class SpkInfoEx: public SpkInfo
{
    char flag;
public:
    SpkInfoEx(unsigned long param1=0, char param2=0):
        SpkInfo(param1), flag(param2)
    {}
    SpkInfoEx(const char* param){
        fromStr(param);
    }
    string toStr() const{
        ostringstream oss;
        oss<<spkId<< "_"<< flag<< ".param";
        return SpkInfo::toStr() + "_" + oss.str();
    }

    bool fromStr(const char* param){
        int ret = sscanf(param, "%lu_%c.param", &spkId, &flag);
        if(ret != 2){ return false; }
        return true;
    }
    bool operator==(const SpkInfo& oth) const;
};
*/

/**
 * 若有wrlock 等待，需要rdlock也等待
 *
 */
class SRWLock{
public:
    explicit SRWLock(){
        wcnt = 0;
        pthread_rwlock_init(&rwlock, NULL);
        pthread_mutex_init(&lock, NULL);
        pthread_cond_init(&cond, NULL);
    }
    ~SRWLock(){
        pthread_rwlock_destroy(&rwlock);
        pthread_mutex_destroy(&lock);
        pthread_cond_destroy(&cond);
    }
    void wlock(){
        pthread_mutex_lock(&lock);
        wcnt ++;
        pthread_mutex_unlock(&lock);
        pthread_rwlock_wrlock(&rwlock);
        pthread_mutex_lock(&lock);
        wcnt --;
        pthread_mutex_unlock(&lock);
        pthread_cond_broadcast(&cond);
    }
    void unlock(){
        pthread_rwlock_unlock(&rwlock);
    }
    void rlock(){
        pthread_mutex_lock(&lock);
        while(wcnt > 0){
            pthread_cond_wait(&cond, &lock);
        }
        pthread_mutex_unlock(&lock);
        pthread_rwlock_rdlock(&rwlock);
    }

    pthread_rwlock_t rwlock;
    pthread_mutex_t lock;
    pthread_cond_t cond;
    int wcnt;
};



class SLockHelper{
public:
    explicit SLockHelper(SRWLock* param, bool exmark = true):
        plock(param)
    {
        if(exmark) param->wlock();
        else param->rlock();
    }

    ~SLockHelper(){
        plock->unlock();
    }

private:
    SLockHelper(const SLockHelper&);
    SLockHelper& operator=(const SLockHelper&);
    SRWLock* plock;
};

struct SpkCheckBook{
    explicit SpkCheckBook(SpkInfo* s=NULL, int i=-1):
        spk(s), idx(i)
    {}
    SpkInfo *spk;
    int idx;
    set<SpkInfo*> oldSpks;
};

static float defaultSpkScoreThrd = 0.0;
static vector<void*> g_vecSpeakerModels;
static vector<unsigned long> g_vecSpeakerIDs;
static map<unsigned long, SpkCheckBook > g_mapAllSpks;
//static float spkex_ScoreThreshold = 0.0001;
//static pthread_rwlock_t g_SpkInfoRwlock = PTHREAD_RWLOCK_INITIALIZER;
static SRWLock g_SpkSLock;

void setDefSpkThrd(float score)
{
    SLockHelper lock(&g_SpkSLock);
    defaultSpkScoreThrd = score;
}

void spkex_getAllSpks(vector<const SpkInfo*> &outSpks)
{
    SLockHelper mylock(&g_SpkSLock, false);
    outSpks.clear();
    for(unsigned idx=0; idx < g_vecSpeakerIDs.size(); idx++){
        outSpks.push_back(g_mapAllSpks[g_vecSpeakerIDs[idx]].spk);
    }
}

bool spkex_addSpk(SpkInfo* spk, char* mdlData, unsigned mdlLen)
{
    string fname = "ioacas/SpkModel/";
    fname += spk->toStr();
    TITStatus titret = TIT_SPKID_SAVE_MDL_IVEC(mdlData, fname.c_str());
    if(titret != StsNoError)
    {
        SLOGE_FMT("in spkex_addSpk, failed to call TIT_SPKID_SAVE_MDL_IVEC, error: %d.", titret);
        return false;
    }
    void *spkmdl = NULL;
    titret = TIT_SPKID_LOAD_MDL_IVEC(spkmdl, fname.c_str());
    if(titret != StsNoError){
        SLOGE_FMT("in spkex_addSpk, failed to call TIT_SPKID_LOAD_MDL_IVEC, error: %d.", titret);
        return false;
    }

    {
        SLockHelper mylock(&g_SpkSLock);
        const unsigned long &spkId = spk->spkId;
        int spkIdx = -1;
        if(g_mapAllSpks.find(spkId) != g_mapAllSpks.end()){
            SpkInfo *oldSpk = g_mapAllSpks[spkId].spk;
            if(oldSpk != NULL){
                spkIdx = g_mapAllSpks[spkId].idx;
                TIT_SPKID_DEL_MDL(g_vecSpeakerModels[spkIdx]);
                if(oldSpk != spk){
                    if(oldSpk->add(0) > 0){
                        g_mapAllSpks[spkId].oldSpks.insert(oldSpk);   
                    }
                    else{
                        delete oldSpk;
                    }
                    g_mapAllSpks[spkId].spk = spk;
                }
            }
        }
        else{
            spkIdx = g_vecSpeakerModels.size();
            g_mapAllSpks.insert(make_pair(spkId, SpkCheckBook(spk, spkIdx)));
            g_vecSpeakerModels.push_back(NULL);
            g_vecSpeakerIDs.push_back(spkId);
        }
        g_vecSpeakerModels[spkIdx] = spkmdl;
    }
    return true;
}

bool spkex_rmSpk(unsigned long spkId)
{
    SLockHelper mylock(&g_SpkSLock);
    if(g_mapAllSpks.find(spkId) != g_mapAllSpks.end() && g_mapAllSpks[spkId].spk != NULL){
        SpkInfo *&delSpk = g_mapAllSpks[spkId].spk;
        int spkIdx = g_mapAllSpks[spkId].idx;
        set<SpkInfo*> oldSpks = g_mapAllSpks[spkId].oldSpks;
        if(delSpk->add(0) == 0){
            delete delSpk;
        }
        else{
            oldSpks.insert(delSpk);
        }
        delSpk = NULL;
        if(oldSpks.size() == 0){
            g_mapAllSpks.erase(spkId);
        }
        TITStatus titret = TIT_SPKID_DEL_MDL(g_vecSpeakerModels[spkIdx]);
        if(titret != StsNoError){
            SLOGE_FMT("in spkex_rmSpk, failed to call TIT_SPKID_DEL_MDL, error: %d.", titret);
        }
        g_vecSpeakerModels.erase(g_vecSpeakerModels.begin() + spkIdx);
        g_vecSpeakerIDs.erase(g_vecSpeakerIDs.begin() + spkIdx);
        for(unsigned idx=spkIdx; idx < g_vecSpeakerIDs.size(); idx++){
            unsigned long curid = g_vecSpeakerIDs[idx];
            assert(g_mapAllSpks.find(curid) != g_mapAllSpks.end());
            g_mapAllSpks[curid].idx = idx;
        }
    }
    else{
        SLOGW_FMT("in removeSpeaker, no spk found in spk array. spk: %lu.", spkId);
    }
    return true;
}


unsigned spkex_getAllSpks(std::vector<unsigned long> &spkIds)
{
    SLockHelper mylock(&g_SpkSLock, false);
    spkIds.clear();
    spkIds.insert(spkIds.begin(), g_vecSpeakerIDs.begin(), g_vecSpeakerIDs.end());
    return g_vecSpeakerIDs.size();
}

#define MAX_PATH 512
//static char g_ModelPath[MAX_PATH];
//static char g_CfgFile[MAX_PATH];
bool spkex_init(const char* cfgfile)
{
    openspkeng();
    //SLockHelper mylock(&g_SpkInfoRwlock);
    TITStatus err = TIT_SPKID_INIT(cfgfile);
    if(err != StsNoError){
        SLOGE_FMT("in intSpkRec, fail to initialize spk engine. error: %d; file: %s.", err, cfgfile);
        return false;
    }
    return true;
}

void spkex_rlse()
{
    vector<unsigned long> allspks;
    spkex_getAllSpks(allspks);
    for(unsigned idx=0; idx < allspks.size(); idx++){
        spkex_rmSpk(allspks[idx]);
    }

    TITStatus err = TIT_SPKID_EXIT();
    if(err != StsNoError){
        SLOGE_FMT("in rlseSpkRec, failed to release spk engine. error: %d.\n", StsNoError);
    }
}

/**
 * 
 * return 0 if no speaker exist, 1 no error occur, -1 failed in engine.
 */
int spkex_score(short* pcmData, unsigned smpNum, const SpkInfo* &spk, float &score)
{
    SLockHelper mylock(&g_SpkSLock, false);
    spk = NULL;
    score = -1000.0;
    if(g_vecSpeakerModels.size() == 0){
        
        return 0;
    }
    vector<float> vecScores;
    size_t tIdx = 0;
    vecScores.resize(g_vecSpeakerModels.size());
    for(size_t idx=0; idx < vecScores.size(); idx ++){
        vecScores[idx] = -1000.0;
    }
    TITStatus err = TIT_SPKID_VERIFY_CLUSTER(pcmData, smpNum, const_cast<const void **>(&g_vecSpeakerModels[0]), g_vecSpeakerModels.size(), &vecScores[0]);
    if(err != StsNoError){
        SLOGE_FMT("in processSpkRec, failed in spk engine. error: %d.", err);
        return -1;
    }
    for(size_t idx=0; idx < vecScores.size(); idx++){
        if(score < vecScores[idx]){
            tIdx = idx;
            score = vecScores[idx];
        }
    }
    if(score > defaultSpkScoreThrd){
        unsigned long spkid = g_vecSpeakerIDs[tIdx];
        SpkInfo *tmpspk = g_mapAllSpks[spkid].spk;
        tmpspk->add(1);
        spk = tmpspk;
    }
    return 1;
}

void returnSpkInfo(const SpkInfo *spk)
{
    int retcnt = 1;
    {
        SLockHelper mylock(&g_SpkSLock, false);
        SpkCheckBook & b = g_mapAllSpks[spk->spkId];
        if(b.spk == spk){
            assert(b.spk->add(-1) >= 0);
        }
        else{
            set<SpkInfo*>::iterator spkit = b.oldSpks.find(const_cast<SpkInfo*>(spk));
            assert(spkit != b.oldSpks.end());
            retcnt = (*spkit)->add(-1);
            assert(retcnt >=0);
        }
    }
    if(retcnt == 0){
        SLockHelper mylock(&g_SpkSLock);
        SpkCheckBook &c = g_mapAllSpks[spk->spkId];
        assert(c.oldSpks.erase(const_cast<SpkInfo*>(spk)) == 1);
        if(c.oldSpks.size() == 0 && c.spk == NULL){
            g_mapAllSpks.erase(spk->spkId);
        }
    }
}
