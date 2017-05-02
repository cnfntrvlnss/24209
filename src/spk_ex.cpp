/*************************************************************************
    > File Name: spk_ex.cpp
    > Author: zhengshurui
    > Mail:  zhengshurui@thinkit.cn
    > Created Time: Sun 09 Oct 2016 07:27:23 PM PDT
 ************************************************************************/

#include "spk_ex.h"

#include <pthread.h>
#include <cassert>
#include <cstdio>

#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <list>
#include <map>
#include <set>

#include "TIT_SPKID_OFFL_API.h"

using namespace std;
#define SLOGE_FMT(fmt, ...) fprintf(stderr, "ERROR " fmt "\n", ##__VA_ARGS__)
#define SLOGW_FMT(fmt, ...) fprintf(stderr, "WARN " fmt "\n", ##__VA_ARGS__)

struct RefCounterImpl: public RefCounter
{
    RefCounterImpl(){
        cnt = 0;
        pthread_mutex_init(&lock, NULL);
    }
    ~RefCounterImpl(){
        pthread_mutex_destroy(&lock);
    }
    void incr(){
        pthread_mutex_lock(&lock);
        cnt ++;
        pthread_mutex_unlock(&lock);
    }
    int decr(){
        pthread_mutex_lock(&lock);
        int ret = cnt --;
        pthread_mutex_unlock(&lock);
        return ret;
    }
    int get(){
        pthread_mutex_lock(&lock);
        int ret = cnt;
        pthread_mutex_unlock(&lock);
        return ret;
    }

    pthread_mutex_t lock;
    int cnt;
private:
    RefCounterImpl(const RefCounterImpl&);
    RefCounterImpl& operator=(const RefCounterImpl&);
};

RefCounter* RefCounter::getInstance()
{
    return new RefCounterImpl();   
}

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

/*
bool SpkInfoEx::operator==(const SpkInfo& oth) const{
    if(typeid(*this) != typeid(oth)){
        return false;
    }
    const SpkInfoEx& refOth = static_cast<const SpkInfoEx&>(oth);
    return SpkInfo::operator==(oth) && flag == refOth.flag;
}
*/
//vector<const SpkInfo*> g_vecSpkInfoPtr;

struct SpkCheckBook{
    explicit SpkCheckBook(SpkInfo* s=NULL, int i=-1):
        spk(s), idx(i)
    {}
    SpkInfo *spk;
    int idx;
    set<SpkInfo*> oldSpks;
};

static int g_FeatSize;
static int g_ModelSize;
vector<void*> g_vecSpeakerModels;
vector<unsigned long> g_vecSpeakerIDs;
map<unsigned long, SpkCheckBook > g_mapAllSpks;
static float spkex_ScoreThreshold = 0.0001;
pthread_rwlock_t g_SpkInfoRwlock = PTHREAD_RWLOCK_INITIALIZER;

static string g_strSpkMdlDir = "ioacas/SpkModel/";

class SLockHelper{
public:
    SLockHelper(pthread_rwlock_t* param, bool exmark = true):
        plock(param), bRdlock(exmark)
    {
        if(exmark) pthread_rwlock_wrlock(plock);
        else pthread_rwlock_rdlock(plock);
    }

    ~SLockHelper(){
        pthread_rwlock_unlock(plock);
    }

private:
    SLockHelper(const SLockHelper&);
    SLockHelper& operator=(const SLockHelper&);
    pthread_rwlock_t* plock;
    bool bRdlock;
};

void spkex_getAllSpks(vector<const SpkInfo*> &outSpks)
{
    SLockHelper mylock(&g_SpkInfoRwlock, false);
    outSpks.clear();
    for(unsigned idx=0; idx < g_vecSpeakerIDs.size(); idx++){
        outSpks.push_back(g_mapAllSpks[g_vecSpeakerIDs[idx]].spk);
    }
}

bool spkex_addSpk(SpkInfo* spk, char* mdlData, unsigned mdlLen)
{
    SLockHelper mylock(&g_SpkInfoRwlock);

    char *pData = (char*)malloc(g_ModelSize);
    TIT_RET_CODE err = TIT_Feat_To_Model(mdlData, pData);
    if(err != TIT_SPKID_SUCCESS){
        SLOGE_FMT("in addSpeaker, failed to call TIT_Feat_To_Model, TitError: %d", err);
    }
    const unsigned long &spkId = spk->spkId;
    int spkIdx = -1;
    if(g_mapAllSpks.find(spkId) != g_mapAllSpks.end()){
        SpkInfo *oldSpk = g_mapAllSpks[spkId].spk;
        if(oldSpk != NULL){
            spkIdx = g_mapAllSpks[spkId].idx;
            //TIT_SPKID_DEL_MDL(g_vecSpeakerModels[spkIdx]);
            free(g_vecSpeakerModels[spkIdx]);
        }
        if(oldSpk != spk){
            if(oldSpk->cnter->get() > 0){
                g_mapAllSpks[spkId].oldSpks.insert(oldSpk);   
            }
            else{
                delete oldSpk;
            }
            g_mapAllSpks[spkId].spk = spk;
        }
    }
    else{
        spkIdx = g_vecSpeakerModels.size();
        g_mapAllSpks.insert(make_pair(spkId, SpkCheckBook(spk, spkIdx)));
        g_vecSpeakerModels.push_back(NULL);
        g_vecSpeakerIDs.push_back(spkId);
    }
    g_vecSpeakerModels[spkIdx] = pData;
    return true;
}

bool spkex_rmSpk(unsigned long spkId)
{
    SLockHelper mylock(&g_SpkInfoRwlock);
    if(g_mapAllSpks.find(spkId) != g_mapAllSpks.end() && g_mapAllSpks[spkId].spk != NULL){
        SpkInfo *&delSpk = g_mapAllSpks[spkId].spk;
        int spkIdx = g_mapAllSpks[spkId].idx;
        set<SpkInfo*> oldSpks = g_mapAllSpks[spkId].oldSpks;
        if(delSpk->cnter->get() == 0){
            delete delSpk;
        }
        else{
            oldSpks.insert(delSpk);
        }
        delSpk = NULL;
        if(oldSpks.size() == 0){
            g_mapAllSpks.erase(spkId);
        }
        //TIT_SPKID_DEL_MDL(g_vecSpeakerModels[spkIdx]);
        free(g_vecSpeakerModels[spkIdx]);
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
    SLockHelper mylock(&g_SpkInfoRwlock, false);
    spkIds.clear();
    spkIds.insert(spkIds.begin(), g_vecSpeakerIDs.begin(), g_vecSpeakerIDs.end());
    return g_vecSpeakerIDs.size();
}

#define MAX_PATH 512
static char g_ModelPath[MAX_PATH];
static char g_CfgFile[MAX_PATH];
bool spkex_init(const char* cfgfile)
{
    SLockHelper mylock(&g_SpkInfoRwlock);
    /*
    TITStatus err = TIT_SPKID_INIT(cfgfile);
    if(err != StsNoError){
        SLOGE_FMT("in intSpkRec, fail to initialize spk engine. error: %d; file: %s.", err, cfgfile);
        return false;
    }
    */
    int indexSize;
    snprintf(g_ModelPath, MAX_PATH, ".");
    snprintf(g_CfgFile, MAX_PATH, cfgfile);
    TIT_RET_CODE err = TIT_SPKID_Init(g_CfgFile, g_ModelPath, g_FeatSize, g_ModelSize, indexSize);
    if(err != TIT_SPKID_SUCCESS){
        SLOGE_FMT("in spkex_init, faile to TIT_SPKID_Init, TitError: %d", err);
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

    /*
    TITStatus err = TIT_SPKID_EXIT();
    if(err != StsNoError){
        SLOGE_FMT("in rlseSpkRec, failed to release spk engine. error: %d.\n", StsNoError);
    }
    */
    TIT_RET_CODE err = TIT_SPKID_Exit();
    if(err != TIT_SPKID_SUCCESS){
        SLOGE_FMT("in spkex_rlse faied to TIT_SPKID_Exit, TitError: %d", err);
    }
}

/**
 * 
 * return 0 if no speaker exist, 1 no error occur, -1 failed in engine.
 */
int spkex_score(short* pcmData, unsigned smpNum, const SpkInfo* &spk, float &score)
{
    SLockHelper mylock(&g_SpkInfoRwlock, false);
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
    /*
    TITStatus err = TIT_SPKID_VERIFY_CLUSTER(pcmData, smpNum, const_cast<const void **>(&g_vecSpeakerModels[0]), g_vecSpeakerModels.size(), &vecScores[0]);
    if(err != StsNoError){
        SLOGE_FMT("in processSpkRec, failed in spk engine. error: %d.", err);
        return -1;
    }
    for(size_t idx=0; idx < vecScores.size(); idx++){
        if(score < vecScores[idx]){
            tIdx = idx;
        }
    }
    */
    int target = -1;
    float* scores = &vecScores[0];
    TIT_RET_CODE err = TIT_SCR_Buf_AddCfd_CutSil_Cluster(pcmData, smpNum, const_cast<void **>(&g_vecSpeakerModels[0]), scores, g_vecSpeakerModels.size(), target
        );
    if(err != TIT_SPKID_SUCCESS){
        SLOGE_FMT("in spkex_score faied to TIT_SCR_Buf_AddCfd_CutSil_Cluster, TitError: %d", err);
    }
    if(target < 0){
        return 1;
    }
    tIdx = target;
    score = vecScores[tIdx];
    if(score >= spkex_ScoreThreshold){
        unsigned long spkid = g_vecSpeakerIDs[tIdx];
        spk = g_mapAllSpks[spkid].spk;
        spk->cnter->incr();
    }
    return 1;
}

void returnSpkInfo(const SpkInfo *spk)
{
    bool bdel = false;
    pthread_rwlock_rdlock(&g_SpkInfoRwlock);
    SpkCheckBook & b = g_mapAllSpks[spk->spkId];
    if(b.spk == spk){
        assert(spk->cnter->decr() >= 0);
    }
    else{
        assert(b.oldSpks.find(const_cast<SpkInfo*>(spk)) != b.oldSpks.end());
        if(spk->cnter->get() == 1){
            bdel = true;
        }
    }
    pthread_rwlock_unlock(&g_SpkInfoRwlock);
    if(bdel){
        pthread_rwlock_wrlock(&g_SpkInfoRwlock);
        assert(spk->cnter->decr() == 0);
        SpkCheckBook &c = g_mapAllSpks[spk->spkId];
        assert(c.oldSpks.erase(const_cast<SpkInfo*>(spk)) == 1);
        if(c.oldSpks.size() == 0 && c.spk == NULL){
            g_mapAllSpks.erase(spk->spkId);
        }
        pthread_rwlock_unlock(&g_SpkInfoRwlock);
    }
}
