/*************************************************************************
    > File Name: spk_ex.h
    > Author: zhengshurui
    > Mail:  zhengshurui@thinkit.cn
    > Created Time: Tue 11 Oct 2016 02:03:38 AM PDT
 ************************************************************************/

#ifndef SPK__EX_H
#define SPK__EX_H

#include <vector>
#include <string>
#include <typeinfo>

struct RefCounter{
    static RefCounter* getInstance();
    virtual void incr() =0;
    virtual int decr() =0;
    virtual int get() = 0;
    virtual ~RefCounter(){}
};

class SpkInfo{
public:
    explicit SpkInfo(unsigned long param = 0):
        spkId(param)
    {
        cnter = RefCounter::getInstance();
    }
    explicit SpkInfo(const char* param)
    {
        fromStr(param);
        cnter = RefCounter::getInstance();
    }
    virtual ~SpkInfo(){
        delete cnter;
    }
    virtual std::string toStr() const;
    virtual bool fromStr(const char* );
public:
    unsigned long spkId;
    RefCounter *cnter;
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

#endif
