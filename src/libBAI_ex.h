/*************************************************************************
	> File Name: libBAI_ex.h
	> Author: 
	> Mail: 
	> Created Time: Thu 09 Jun 2016 03:35:15 AM CST
 ************************************************************************/

#ifndef _LIBBAI_EX_H
#define _LIBBAI_EX_H

#include "ProjectBuffer.h"

#include <vector>
#include <iostream>
#include <time.h>
#include "commonFunc.h"
#include "../include/interface242.h"

struct BampMatchParam{
    explicit BampMatchParam(unsigned long param1, zen4audio::ProjectBuffer *param2):
        pid(param1),  bHit(false), ptrBuf(param2)
    {}
    unsigned long pid;
    bool bPreHit;
    unsigned preIdx;
    unsigned preOffset;
    unsigned preLen;
    unsigned endIdx;
    unsigned endOffset;
    unsigned tolLen;
    std::vector<DataBlock> data;
    bool bHit;
    zen4audio::ProjectBuffer *ptrBuf;
    struct timeval curtime;
    //unsigned targetID;
};

struct BampMatchObject{
    BampMatchObject(const char *libfile);
    ~BampMatchObject();
    pthread_t threadId;
    void* hdl;
    bool bHasModel;
    bool bOpened;
    LockHelper lock;
    bool loadModel(const char* libFile);
    bool bamp_match(unsigned long pid, char *pcm1, unsigned len1, unsigned preLen, struct timeval curtime);
    bool bamp_match(std::vector<BampMatchParam>& allData);
    bool bamp_match_vad(std::vector<BampMatchParam>& allData);
private:
    BampMatchObject(const BampMatchObject&);
    BampMatchObject& operator=(const BampMatchObject&);
};

BampMatchObject* openBampHandle();
/*
inline bool bamp_match(unsigned long pid, char *pcm1, unsigned len1, unsigned preLen, struct timeval curtime)
{
    BampMatchObject *obj = openBampHandle();
    if(obj) return obj->bamp_match(pid, pcm1, len1, preLen, curtime);
    else return false;
}
*/

extern char g_szBampIp[50];
extern unsigned short g_uBampPort;
extern float g_fReportBampThrd;
extern unsigned g_uBampThreadNum;
extern const unsigned short g_uBampFDServType;
extern const unsigned short g_uBampJCServType;

struct BampResultParam{
    CDLLResult *pResult;
    struct timeval curtime;
    bool bPreHit;
    zen4audio::ProjectBuffer* ptrBuf;
};
typedef void (*SummitBampResult)(BampResultParam param, std::ostream& oss);
//bool bamp_init(SummitBampResult callbck);
bool bamp_init(SummitBampResult callbck, unsigned vadParallelNum=1, float afterVadRatio=0.05);
bool bamp_rlse();
//bool bamp_match(unsigned long pid, short *pcmData, unsigned pcmLen, unsigned preLen, struct timeval curtime);
//bool bamp_match(unsigned long pid, char *pcm1, unsigned len1, unsigned preLen, struct timeval curtime);
//void *OnecircleOffline(void *param);

#endif
