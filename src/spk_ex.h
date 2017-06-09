/*************************************************************************
    > File Name: spk_ex.h
    > Author: zhengshurui
    > Mail:  zhengshurui@thinkit.cn
    > Created Time: Tue 11 Oct 2016 02:03:38 AM PDT
 ************************************************************************/

#ifndef SPK__EX_H
#define SPK__EX_H

#include <pthread.h>
#include <vector>
#include <string>
#include <typeinfo>

class SpkInfo{
public:
    explicit SpkInfo(unsigned long param = 0):
        spkId(param)
    {
        refcnt = 0;
        pthread_mutex_init(&lock, NULL);
    }
    explicit SpkInfo(const char* param)
    {
        fromStr(param);
        refcnt = 0;
    }
    virtual ~SpkInfo(){
        pthread_mutex_destroy(&lock);
    }
    virtual std::string toStr() const;
    virtual bool fromStr(const char* );

    int add(int step){
        pthread_mutex_lock(&lock);
        refcnt += step;
        int ret = refcnt;
        pthread_mutex_unlock(&lock);
        return ret;
    }
public:
    unsigned long spkId;
    pthread_mutex_t lock;
    int refcnt;
private:
    SpkInfo(const SpkInfo&);
    SpkInfo& operator=(const SpkInfo&);
};

//unsigned spkex_getAllSpks(std::vector<const SpkInfo*> &spks);
unsigned spkex_getAllSpks(std::vector<unsigned long> &spkIds);
const SpkInfo* getSpk(unsigned long);
void returnSpkInfo(const SpkInfo*);
bool spkex_addSpk(SpkInfo* spk, char* mdlData, unsigned mdlLen);
//bool spkex_rmSpk(const SpkInfo* spk, const SpkInfo* oldSpk);
bool spkex_rmSpk(unsigned long spkId);
bool spkex_init(const char* cfgfile);
void spkex_rlse();
int spkex_score(short* pcmData, unsigned smpNum, const SpkInfo* &spk, float &score);
void returnSpkInfo(const SpkInfo *spk);
void setDefSpkThrd(float score);

#endif
