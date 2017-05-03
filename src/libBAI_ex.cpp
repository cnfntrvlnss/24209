/*************************************************************************
    > File Name: libBAI_ex.cpp
    > Author: 
    > Mail: 
    > Created Time: Wed 08 Jun 2016 09:26:26 PM CST
 ************************************************************************/

#include "libBAI_ex.h"
#include "dllSRVADCluster_lshj.h"
#include <fcntl.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <poll.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <pthread.h>
#include <unistd.h>

#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <cerrno>

#include <cassert>
#include <map>
#include <vector>
#include <string>
#include<iostream>
#include <sstream>
using namespace std;

#include "utilites.h"
#include "wav/Voc2Wav.h"
#include "../include/interface242.h"
#include "libBAI.h"
#include "socket_ex.h"
#include "commonFunc.h"

#include "protosrc/2.6/FixedAudioModel.pb.h"
using namespace FixedAudioModel;

#define MAX_PATH 512

char g_szBampIp[50] = "";
unsigned short g_uBampPort = 10019;
const unsigned MAX_BUF = 1024;
const char g_TempDir[MAX_PATH] = "temp/";
float g_fReportBampThrd = 86;

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

struct SampleInfoStruct{
    string fname;
    bool bUpdated;
};

SummitBampResult funcBampSubmitResult;
const unsigned short g_uBampFDServType = 0xc8;
const unsigned short g_uBampJCServType = 0xc3;
static char g_szBampCfgFile[MAX_PATH] = "ioacas/Bamp.cfg";
unsigned g_uBampThreadNum = 1;
static char g_szBampLibFile[MAX_PATH] = "ioacas/output/database.dat";
//static time_t g_BampLibLastTime = 0;

BampMatchObject::BampMatchObject(const char* libFile)
{
    this->threadId = pthread_self();
    this->hdl = NULL;
    while(true){
        BAI_Code err = BAI_Open(&(this->hdl));
        if(err != BAI_OK){
            BLOGE("failed to open one session of bamp match, err: %d.", err);
            this->bOpened = false;
            break;
        }
        this->bOpened = true;
        this->bHasModel = false;
        FILE *fp = fopen(libFile, "rb");
        if(fp == NULL) break;
        fclose(fp);
        err = BAI_LoadIndex(libFile, &(this->hdl));
        if(err != BAI_OK){
            BLOGE("failed to load libfile %s, err: %d.", libFile, err);
            break;
        }
        this->bHasModel = true;
        break;
    }
    
    BLOGI("have created one BampMatch object, handle: %lx; thread: %lu; bOpened: %d; bHasModel: %d.", this->hdl, this->threadId, this->bOpened, this->bHasModel);
}
BampMatchObject::~BampMatchObject()
{
    AutoLock(this->lock);
    BAI_Code err = BAI_Close(&(this->hdl));
    if(err != BAI_OK){
        BLOGE("failed to close session %x, err: %d.", this->hdl, err);
    }
}

bool BampMatchObject::loadModel(const char *libFile)
{
    AutoLock(this->lock);
    BAI_Code err = BAI_LoadIndex(libFile, &(this->hdl));
    if(err != BAI_OK){
        BLOGE("failed to load libfile %s, err: %d.", libFile, err);
        return false;
    }
    if(!this->bHasModel) this->bHasModel = true;
    return true;
}

struct EngInputCompanyParam{
    EngInputCompanyParam(unsigned idx, unsigned segidx, unsigned unitidx, unsigned offset):
    idx(idx), segidx(segidx), unitidx(unitidx), offset(offset)
    {}
    unsigned idx;
    unsigned segidx;
    unsigned unitidx;
    unsigned offset;
};

struct InputVADParam{
    InputVADParam(const char*param1, unsigned param2, unsigned param3, unsigned param4):
        buf(param1), len(param2), projIdx(param3), segIdx(param4), isValid(false)
    {}
    const char *buf;
    unsigned len;
    unsigned projIdx;
    unsigned segIdx;
    bool isValid;
};

struct PreBampEngVADTask{
    PreBampEngVADTask(){
        this->len = 0;
        this->taskArr = NULL;
        pthread_mutex_init(&lock, NULL);
        pthread_cond_init(&cond, NULL);
    }
    ~PreBampEngVADTask(){
        pthread_mutex_destroy(&lock);
        pthread_cond_destroy(&cond);
    }
    void setTask(unsigned len, InputVADParam *taskArr){
        pthread_mutex_lock(&lock);
        this->len = len;
        this->taskArr = taskArr;
        pthread_mutex_unlock(&lock);
        pthread_cond_signal(&cond);
    }
    void waitTask(){
        pthread_mutex_lock(&lock);
        while(len == 0){
            pthread_cond_wait(&cond, &lock);
        }
        pthread_mutex_unlock(&lock);
    }
    void notifyFinish(){
        pthread_mutex_lock(&lock);
        len = 0;
        pthread_mutex_unlock(&lock);
        pthread_cond_signal(&cond);
    }
    void waitFinish(){
        pthread_mutex_lock(&lock);
        while(len != 0){
            pthread_cond_wait(&cond, &lock);
        }
        pthread_mutex_unlock(&lock);
    }
    pthread_mutex_t lock;
    pthread_cond_t cond;
    unsigned len;
    InputVADParam *taskArr;
private:
    PreBampEngVADTask(const PreBampEngVADTask&);
    PreBampEngVADTask& operator=(const PreBampEngVADTask);
};

#define BAMPSEGMENTLEN 48000
float g_fAfterVadSecs = 0.05;
PreBampEngVADTask *g_pPreBampEngVADTasks = NULL;
unsigned g_uPreBampEngVADTasksLen = 0;
pthread_t *g_PreBampVADThreadIDs = NULL;
void * preBampEngVADThread(void *param)
{
    PreBampEngVADTask *pTask = reinterpret_cast<PreBampEngVADTask*>(param);
    short *tmpBuf = (short*)malloc(BAMPSEGMENTLEN);
    unsigned tmpBufLen = BAMPSEGMENTLEN / 2;
    while(true){
        pTask->waitTask();
        for(size_t idx=0; idx < pTask->len; idx++){
            InputVADParam& task = pTask->taskArr[idx];
            int riSampleNum = tmpBufLen;
            VADBuffer(true, reinterpret_cast<short*>(const_cast<char*>(task.buf)), task.len / 2, tmpBuf, riSampleNum);
            if(!((float)riSampleNum / 16000 < g_fAfterVadSecs)){
                task.isValid = true;
            }
            //if(rand() % 100 < 60) task.isValid = true;
        }
        pTask->notifyFinish();
    }
    free(tmpBuf);
}

void filterByVAD(vector<InputVADParam>& input){
    if(g_uPreBampEngVADTasksLen == 0){
        BLOGD("in filterByVAD, without configuration of Pre-bamp VAD, skip one round.");   
        for(size_t idx=0; idx < input.size(); idx++){
            input[idx].isValid = true;
        }
        return;
    }
    clockoutput_start("filterByVAD");
    InputVADParam *taskArr = &(input[0]); 
    unsigned taskLen = input.size();
    unsigned step = taskLen / g_uPreBampEngVADTasksLen;
    if(taskLen % g_uPreBampEngVADTasksLen != 0) step += 1;
    unsigned thdIdx = 0;
    unsigned taskIdx = 0;
    while(taskIdx < taskLen){
        unsigned curstep;
        if(taskLen - taskIdx > step){
            curstep = step;
        }
        else{
            curstep = taskLen - taskIdx;
        }
        g_pPreBampEngVADTasks[thdIdx].setTask(curstep, &(taskArr[taskIdx]));
        thdIdx++;
        taskIdx += curstep;
    }
    assert(thdIdx <= g_uPreBampEngVADTasksLen);
    for(size_t idx = 0; idx < thdIdx; idx++){
        g_pPreBampEngVADTasks[idx].waitFinish();
    }
    LOGFMTT(clockoutput_end().c_str());
}

bool BampMatchObject::bamp_match_vad(std::vector<BampMatchParam>& allData)
{
    AutoLock mylock(this->lock);
    if(!this->bHasModel){
        BLOGT("in bamp_match no library to use.");
        return false;
    }
    vector<InputVADParam> vadInput;
    for(size_t idx=0; idx < allData.size(); idx++){
        vector<DataBlock>& curData = allData[idx].data;
        for(size_t jdx=0; jdx < curData.size(); jdx++){
            vadInput.push_back(InputVADParam(curData[jdx].getPtr(), curData[jdx].len, idx, jdx));
        }
    }
    filterByVAD(vadInput);
    vector<InputVADParam> vadRet;
    for(size_t idx=0; idx < vadInput.size(); idx++){
        if(vadInput[idx].isValid){
            vadRet.push_back(vadInput[idx]);
        }
        else{
            BampMatchParam &curPrm = allData[vadInput[idx].projIdx];
            unsigned realPreLen = curPrm.preLen + vadInput[idx].segIdx * BAMPSEGMENTLEN;
            BLOGT("saved audio segment treated as noise after pre-Bamp VAD detection, file: %s.", saveProjectSegment(curPrm.curtime, curPrm.pid, &realPreLen,  const_cast<char*>(vadInput[idx].buf), vadInput[idx].len).c_str());
        }
    }
    if(vadRet.size() == 0) return true;
    unsigned intoEngSize = vadRet.size();
    BAI_InputItem *intoEng = new BAI_InputItem[intoEngSize];
    for(size_t idx=0; idx < vadRet.size(); idx++){
        BAI_InputItem &curItem = intoEng[idx];
        curItem.iAudioID = idx;
        //sprintf(curItem.acAudioUrl, "\0");
        curItem.acAudioUrl[0] = '\0';
        curItem.pcDataBuffer = new char[BAMPSEGMENTLEN];
        memcpy(curItem.pcDataBuffer, vadRet[idx].buf, vadRet[idx].len);
        curItem.iBufferSize = vadRet[idx].len;
        curItem.iDataType = 0;

        BampMatchParam &curPrm = allData[vadRet[idx].projIdx];
        unsigned realPreLen = curPrm.preLen + vadRet[idx].segIdx * BAMPSEGMENTLEN;
        BLOGT("saved audio segment before bamp match call, file: %s.", saveProjectSegment(curPrm.curtime, curPrm.pid, &realPreLen,  intoEng[idx].pcDataBuffer, intoEng[idx].iBufferSize).c_str());
    }

    BAI_ResultList *pRes = NULL;
    BAI_Code err = BAI_Retrieval_Partly_VAD(intoEng, intoEngSize, pRes, &(this->hdl));
    if(err != BAI_OK){
        BLOGE("in bamp_match failed to call BAI_Retrieval_Partly, err: %d.", err);
        delete [] intoEng;
        return false;
    }

    CDLLResult desres;
    WavDataUnit desdata;
    desres.m_iDataUnitNum = 1;
    desres.m_pDataUnit[0] = &desdata;
    if(pRes == NULL){
        BLOGE("in bamp_match, no result after BAI_Retrieval.. pRes == NULL.");
        delete [] intoEng;
        return false;
    }
    ostringstream oss;
    for(size_t idx=0; idx < intoEngSize; idx++){
        unsigned iTestID = pRes[idx].iTestID;
        unsigned dataIdx = vadRet[idx].projIdx;
        unsigned unitidxInData = vadRet[idx].segIdx;
        unsigned long pid = allData[dataIdx].pid;
        unsigned len = BAMPSEGMENTLEN;
        unsigned preLen = allData[dataIdx].preLen + unitidxInData * BAMPSEGMENTLEN;
        struct timeval curtime = allData[dataIdx].curtime;
        desdata.m_iPCBID = pid;
        desdata.m_iDataLen = BAMPSEGMENTLEN;
        desdata.m_pData = allData[dataIdx].data[unitidxInData].getPtr();
        oss.str("");
        oss<< "PID=" << pid<< " "<< "WaveLen="<< (len) / 16000<< " Offset="<< preLen / 16000<< " ";
        if(pRes[idx].iResultNum == 0){
            BLOGT("%sbamp_match no result after BAI_Retrieval... err: %d", oss.str().c_str(), pRes[idx].eErrCode);
            continue;
        }
        long stpos = oss.tellp();
        for(size_t jdx=0; jdx < pRes[idx].iResultNum; jdx ++){
            oss.seekp(stpos);
            BAI_ResultItem& curhit = pRes[idx].pstResultItems[jdx];
            if(curhit.fMatchedRate < 1.0001){
                curhit.fMatchedRate *= 100;
            }
            if(curhit.fMatchedRate < g_fReportBampThrd){
                continue;
            }
            oss<< " CfgName="<< curhit.acAudioUrl;
            sscanf(curhit.acAudioUrl, "%u", &desres.m_iTargetID);
            if(desres.m_iTargetID % 2){
                desres.m_iAlarmType = g_uBampJCServType;
            }
            else{
                desres.m_iAlarmType = g_uBampFDServType;
            }
            desres.m_iTargetID /= 2;
            desres.m_iHarmLevel = 0;
            desres.m_fTargetMatchLen = curhit.fDurationS;
            desres.m_fLikely = curhit.fMatchedRate;
            desres.m_fSegLikely[0] = curhit.fMatchedRate;
            desres.m_fSegPosInPCB[0] = ((float)preLen) / 16000 + curhit.fTimeStartInTestS;
            desres.m_fSegPosInTarget[0] = curhit.fTimeStartInWaveS;
            BampResultParam resPrm;
            resPrm.pResult = &desres;
            resPrm.curtime =  curtime;
            resPrm.bPreHit = allData[dataIdx].bPreHit;
            resPrm.ptrBuf = allData[dataIdx].ptrBuf;
            funcBampSubmitResult(resPrm, oss);
            allData[dataIdx].bHit = true;
            oss.put('\0');
            BLOGI("%s", oss.str().c_str());
        }
    }

    delete [] intoEng;
    delete [] pRes;
    return true;
}

bool BampMatchObject::bamp_match(std::vector<BampMatchParam>& allData)
{
    AutoLock mylock(this->lock);
    if(!this->bHasModel){
        BLOGT("in bamp_match no library to use.");
        return false;
    }
    unsigned segNum = 0;
    for(size_t idx=0; idx < allData.size(); idx++){
        segNum += allData[idx].tolLen / BAMPSEGMENTLEN;
        assert(allData[idx].tolLen % BAMPSEGMENTLEN == 0);
    }
    BLOGT("bamp_match job num: %u", segNum);
    BAI_InputItem *intoEng = new BAI_InputItem[segNum];
    unsigned intoEngSize = 0;
    //vector<BAI_InputItem> intoEng;
    vector<EngInputCompanyParam> dataidxarr;
    for(size_t idx=0; idx < allData.size(); idx++){
        BampMatchParam &curPrm = allData[idx];
        const vector<DataBlock>& curData = curPrm.data;
        unsigned unitidx = 0;
        unsigned offset = curData[unitidx].offset;

        unsigned segidx = 0;
        while(true){
            if(unitidx == curData.size() - 1 && offset == curData[unitidx].len + curData[unitidx].offset){
                break;
            }
            if(offset == curData[unitidx].len + curData[unitidx].offset){
                unitidx = unitidx + 1;
                offset = curData[unitidx].offset;
            }

            if(curData[unitidx].len + curData[unitidx].offset - offset >= BAMPSEGMENTLEN){
                assert(intoEngSize < segNum);
                BAI_InputItem &curItem = intoEng[intoEngSize];
                curItem.iAudioID = intoEngSize;
                //sprintf(curItem.acAudioUrl, "\0");
                curItem.acAudioUrl[0] = '\0';
                curItem.pcDataBuffer = new char[BAMPSEGMENTLEN];
                memcpy(curItem.pcDataBuffer, curData[unitidx].getPtr() + offset, BAMPSEGMENTLEN);
                curItem.iBufferSize = BAMPSEGMENTLEN;
                curItem.iDataType = 0;
                intoEngSize ++;
                dataidxarr.push_back(EngInputCompanyParam(idx, segidx, unitidx, offset));
                unsigned realPreLen = curPrm.preLen + segidx * BAMPSEGMENTLEN;
                BLOGT("saved audio segment before bamp match call, file: %s.", saveProjectSegment(curPrm.curtime, curPrm.pid, &realPreLen,  intoEng[intoEngSize - 1].pcDataBuffer, intoEng[intoEngSize - 1].iBufferSize).c_str());
                offset += BAMPSEGMENTLEN;
                segidx ++;
            }
            else{
                assert(curData[unitidx].len + curData[unitidx].offset - offset == 0);
            }
        }
    }

    assert(intoEngSize  == dataidxarr.size());
    BAI_ResultList *pRes = NULL;
    BAI_Code err = BAI_Retrieval_VAD(intoEng, intoEngSize, pRes, &(this->hdl));
    if(err != BAI_OK){
        BLOGE("in bamp_match failed to call BAI_Retrieval_Partly, err: %d.", err);
        delete [] intoEng;
        return false;
    }

    CDLLResult desres;
    WavDataUnit desdata;
    desres.m_iDataUnitNum = 1;
    desres.m_pDataUnit[0] = &desdata;
    if(pRes == NULL){
        BLOGE("in bamp_match, no result after BAI_Retrieval.. pRes == NULL.");
        delete [] intoEng;
        return false;
    }
    ostringstream oss;
    for(size_t idx=0; idx < intoEngSize; idx++){
        unsigned iTestID = pRes[idx].iTestID;
        unsigned dataIdx = dataidxarr[iTestID].idx;
        unsigned segIdxInData = dataidxarr[iTestID].segidx;
        unsigned unitidxInData = dataidxarr[iTestID].unitidx;
        unsigned offsetInData = dataidxarr[iTestID].offset;
        unsigned long pid = allData[dataIdx].pid;
        unsigned len = BAMPSEGMENTLEN;
        unsigned preLen = allData[dataIdx].preLen + segIdxInData * BAMPSEGMENTLEN;
        struct timeval curtime = allData[dataIdx].curtime;
        desdata.m_iPCBID = pid;
        desdata.m_iDataLen = BAMPSEGMENTLEN;
        desdata.m_pData = allData[dataIdx].data[unitidxInData].getPtr() + offsetInData;
        oss.str("");
        oss<< "PID=" << pid<< " "<< "WaveLen="<< (len) / 16000<< " Offset="<< preLen / 16000<< " ";
        if(pRes[idx].iResultNum == 0){
            BLOGT("%sbamp_match no result after BAI_Retrieval... err: %d", oss.str().c_str(), pRes[idx].eErrCode);
            continue;
        }
        long stpos = oss.tellp();
        for(size_t jdx=0; jdx < pRes[idx].iResultNum; jdx ++){
            oss.seekp(stpos);
            BAI_ResultItem& curhit = pRes[idx].pstResultItems[jdx];
            if(curhit.fMatchedRate < 1.0001){
                curhit.fMatchedRate *= 100;
            }
            if(curhit.fMatchedRate < g_fReportBampThrd){
                continue;
            }
            oss<< " CfgName="<< curhit.acAudioUrl;
            sscanf(curhit.acAudioUrl, "%u", &desres.m_iTargetID);
            if(desres.m_iTargetID % 2){
                desres.m_iAlarmType = g_uBampJCServType;
            }
            else{
                desres.m_iAlarmType = g_uBampFDServType;
            }
            desres.m_iTargetID /= 2;
            desres.m_iHarmLevel = 0;
            desres.m_fTargetMatchLen = curhit.fDurationS;
            desres.m_fLikely = curhit.fMatchedRate;
            desres.m_fSegLikely[0] = curhit.fMatchedRate;
            desres.m_fSegPosInPCB[0] = ((float)preLen) / 16000 + curhit.fTimeStartInTestS;
            desres.m_fSegPosInTarget[0] = curhit.fTimeStartInWaveS;
            BampResultParam resPrm;
            resPrm.pResult = &desres;
            resPrm.curtime =  curtime;
            resPrm.bPreHit = allData[dataIdx].bPreHit;
            resPrm.ptrBuf = allData[dataIdx].ptrBuf;
            funcBampSubmitResult(resPrm, oss);
            allData[dataIdx].bHit = true;
            oss.put('\0');
            BLOGI("%s", oss.str().c_str());
        }
    }

    delete [] intoEng;
    delete [] pRes;
    return true;
}

bool BampMatchObject::bamp_match(unsigned long pid, char *pcm1, unsigned len1, unsigned preLen, struct timeval curtime)
{
    AutoLock mylock(this->lock);
    ostringstream oss;
    oss<< "PID=" << pid<< " "<< "WaveLen="<< (len1) / 16000<< " Offset="<< preLen / 16000<< " ";
    if(!this->bHasModel){
        BLOGT("%sbamp_match no library to use.", oss.str().c_str());
        return false;
    }
    unsigned lensz = len1;
    char *pdata = (char*)malloc(lensz);
    if(pdata == NULL){
        BLOGE("%sbamp_match no memory for copy of input data.", oss.str().c_str());
        return false;
    }
    memcpy(pdata, pcm1, len1);
    BAI_InputItem item;
    BAI_ResultList *pRes = NULL;
    item.iAudioID = 0;
    item.acAudioUrl[0] = '\0';
    item.pcDataBuffer = pdata;
    item.iBufferSize = lensz;
    item.iDataType = 0;
    BLOGT("saved audio segment before bamp match call, file: %s.", saveTempBinaryData(curtime, pid, item.pcDataBuffer, item.iBufferSize).c_str());
    BAI_Code err = BAI_Retrieval_Partly_VAD(&item, 1, pRes, &(this->hdl));
    if(err != BAI_OK){
        BLOGE("%sfail to call BAI_Retrieval_Partly, err: %d.", oss.str().c_str(), err);
        return false;
    }

    bool bHit = false;
    CDLLResult desres;
    WavDataUnit desdata;
    desres.m_iDataUnitNum = 1;
    desres.m_pDataUnit[0] = &desdata;
    desdata.m_iPCBID = pid;
    desdata.m_iDataLen = len1;
    desdata.m_pData = pcm1;
    if(pRes == NULL){
        BLOGT("%sbamp_match no retrieval result from library. pRes==NULL.", oss.str().c_str());
    }
    else{
        if(pRes->iResultNum == 0){
            BLOGT("%sbamp_match no retrieval result from library. pRes->iResultNum==0", oss.str().c_str());
        }
        else{
            float fOffset = ((float)preLen) / 16000;
            if(curtime.tv_sec == 0) gettimeofday(&curtime, NULL);
            long stpos = oss.tellp();
            for(size_t idx=0; idx < pRes->iResultNum; idx++){
                BAI_ResultItem &curhit = pRes->pstResultItems[idx];
                if(curhit.fMatchedRate < 1.0001){
                    curhit.fMatchedRate *= 100;
                }
                if(curhit.fMatchedRate < g_fReportBampThrd){
                    continue;
                }
                oss.seekp(stpos);
                oss<< " CfgName=" << curhit.acAudioUrl;
		sscanf(curhit.acAudioUrl, "%u", &desres.m_iTargetID);
                if(desres.m_iTargetID % 2){
                    desres.m_iAlarmType = g_uBampJCServType;
                }
                else{
                    desres.m_iAlarmType = g_uBampFDServType;
                }
                desres.m_iTargetID /= 2;
                desres.m_iHarmLevel = 0;
                desres.m_fTargetMatchLen = curhit.fDurationS;
                desres.m_fLikely = curhit.fMatchedRate;
                desres.m_fSegLikely[0] = curhit.fMatchedRate;
                desres.m_fSegPosInPCB[0] = fOffset + curhit.fTimeStartInTestS;
                desres.m_fSegPosInTarget[0] = curhit.fTimeStartInWaveS;
                //funcBampSubmitResult(curtime, &desres, oss);
                oss.put('\0');
                BLOGI("%s", oss.str().c_str());
                bHit = true;
            }
        }
        
        delete[] pRes;
        pRes = NULL;
    }
    
    return bHit;
}

vector<BampMatchObject* > g_AllBampObjects;
static LockHelper g_BampLock;
static pthread_key_t g_BampHandleKey;
enum BampStatus {UNINITED=0, INITED};
static  BampStatus g_BampStatus = UNINITED;
static pthread_t g_BampThreadID;

static map<unsigned, SampleInfoStruct> g_mBampSamples;

// return true if all handles succeed.
static inline bool bamp_loadNewLib(const char* libFile)
{
    bool ret = true;
    AutoLock lock(g_BampLock);
    vector<BampMatchObject*>::iterator it = g_AllBampObjects.begin();
    for(; it!=g_AllBampObjects.end(); it++){
        if(!(*it)->loadModel(libFile)){
            ret = false;
        }
    }
    return ret;
}

void *buildIndexProcess(void * param);

/****
 * the function must be called between init and rlse, 
 */
BampMatchObject* openBampHandle()
{
    //if(g_BampStatus < INITED){
    //    return NULL;
    //}
    BampMatchObject* ret = (BampMatchObject*)pthread_getspecific(g_BampHandleKey);
    if(ret == NULL){
        ret = new BampMatchObject(g_szBampLibFile);
        if(!ret->bOpened){
            delete ret;
            ret = NULL;
        }
        else{
            pthread_setspecific(g_BampHandleKey, ret);
            AutoLock lock(g_BampLock);
            g_AllBampObjects.push_back(ret);
        }
    }
    return ret;
}
static void closeBampHandle(void* hdl){
    AutoLock lock(g_BampLock);
    BampMatchObject* pHdl = (BampMatchObject*)hdl;
    vector<BampMatchObject*>::iterator it = g_AllBampObjects.begin();
    for(; it != g_AllBampObjects.end(); it++){
        if(*it == pHdl) break;
    }
    if(it == g_AllBampObjects.end()){
        LOGE("in closeBampHandle, handle %x is not an opened handle.");
        assert(false);
    }
    else{
        g_AllBampObjects.erase(it);
        delete pHdl;
    }
}

static void * UpdateLibraryThread(void *param);
bool bamp_init(SummitBampResult callbck, unsigned vadParallelNum, float afterVadRatio)
{
    AutoLock mylock(g_BampLock);
    BLOGI("in bamp_init, starting init BAI.");
    BAI_Code err = BAI_Init(g_szBampCfgFile, g_uBampThreadNum);
    if(err != BAI_OK){
      BLOGE("failed to initialize  bamp engine. err: %d.", err);   
        return false;
    }
    int ierr = pthread_key_create(&g_BampHandleKey, closeBampHandle);
    if(ierr != 0){
        LOG_ERROR(g_logger, "fail to create BampHandleKey, error: "<< ierr);
        return false;
    }
    funcBampSubmitResult = callbck;
    g_BampStatus = INITED;
    BLOGI("in bamp_init, finishing init BAI.");
    int retcrt;
    g_uPreBampEngVADTasksLen = vadParallelNum;
    if(g_uPreBampEngVADTasksLen > 0){
        if(!InitVADCluster_File()){
            BLOGE("in bamp_init, faile to initVADCluster_File.");
            g_uPreBampEngVADTasksLen = 0;
        }
        else{
            g_fAfterVadSecs = afterVadRatio;
            g_pPreBampEngVADTasks = new PreBampEngVADTask[g_uPreBampEngVADTasksLen];
            g_PreBampVADThreadIDs = new pthread_t[g_uPreBampEngVADTasksLen];
            for(size_t idx=0; idx < g_uPreBampEngVADTasksLen; idx++){
                retcrt = pthread_create(&(g_PreBampVADThreadIDs[idx]), NULL, preBampEngVADThread, &(g_pPreBampEngVADTasks[idx]));
                if(retcrt != 0){
                    BLOGE("fail to create UpdateLibraryThread. err: %d", retcrt);
                    exit(1);
                }
                BLOGI("in bamp_init, preBampEngVADThread %u has been created.", idx);
            }
        }
    }
    
    retcrt = pthread_create(&g_BampThreadID, NULL, UpdateLibraryThread, NULL);
    if(retcrt != 0){
        BLOGE("fail to create UpdateLibraryThread. err: %d", retcrt);
    }
    BLOGI("in bamp_init, UpdateLibrary thread has been created.");
    return true;
}

bool bamp_rlse()
{
    AutoLock mylock(g_BampLock);
    pthread_cancel(g_BampThreadID);
    pthread_join(g_BampThreadID, NULL);
    BLOGI("in bamp_rlse, updateLibraryThread has been canceled.");
    if(g_uPreBampEngVADTasksLen > 0){
        for(size_t idx=0; idx < g_uPreBampEngVADTasksLen; idx++){
            pthread_cancel(g_PreBampVADThreadIDs[idx]);
            pthread_join(g_PreBampVADThreadIDs[idx], NULL);
        }
        delete [] g_pPreBampEngVADTasks;
        delete [] g_PreBampVADThreadIDs;
        FreeVADCluster();
        BLOGI("in bamp_rlse, shutdown PreBampVAD module.");
    }

    if(g_AllBampObjects.size() > 0){
        BLOGI("in bamp_rlse, there are %u matchObjects to be deleted.", g_AllBampObjects.size());
        for(size_t idx=0; idx < g_AllBampObjects.size(); idx++){
            delete g_AllBampObjects[idx];
        }
        g_AllBampObjects.clear();
        BLOGI("in bamp_rlse, all matchObject has been deleted.");
    }
    pthread_key_delete(g_BampHandleKey);
    g_BampStatus = UNINITED;
    BAI_Code err = BAI_Exit();
    if(err != BAI_OK){
        BLOGE("failed to exit bamp. err: %d", err);
        return false;
    }       
    BLOGI("in bamp_rlse, Bamp engine has exited.");
    return true;
}
/*
static void process_bai_resultlist(unsigned long pid, short* pcmData, unsigned pcmLen, BAI_ResultList* pres)
{
}
*/

static bool saveBinaryFile(const char* fname, char *pData, unsigned dataLen)
{
    FILE *fp = fopen(fname, "wb");
    if(fp == NULL){
        BLOGE("saveBinaryFile failed to create file %s.", fname);
        return false;
    }
    size_t wlen = fwrite(pData, 1, dataLen, fp);
    bool ret = true;
    if(wlen != dataLen){
        BLOGE("saveBinaryFile write part data to file, total: %u, writen: %lu, file: %s", dataLen, wlen, fname);
        ret = false;
    }
    fclose(fp);
    return ret;
}

void addNewSmp(unsigned int id, const char *fname)
{
    if(g_mBampSamples.find(id) != g_mBampSamples.end()){
        if(g_mBampSamples[id].fname == fname){
            BLOGD("addNewTemp TempID=%u already exists in Bamp library.", id);
            g_mBampSamples[id].bUpdated = false;
            return; 
        } 
    }
    else{
        g_mBampSamples[id].fname = fname;
        g_mBampSamples[id].bUpdated = true;
    }
}

void updateBampLib(const char *listFile)
{
    for(map<unsigned, SampleInfoStruct>::iterator it = g_mBampSamples.begin(); it != g_mBampSamples.end(); it ++){
        it->second.bUpdated = true;
    }

    vector<string> smpList = loadFileList(listFile);
    for(size_t idx = 0; idx < smpList.size(); idx ++){
        unsigned smpId;
        char smpPath[MAX_PATH];
        if(sscanf(smpList[idx].c_str(), "%u %s", &smpId, smpPath) != 2){
            BLOGE("offlineThread failed to parse line to get sample, %s.", smpList[idx].c_str());
            continue;
        }
        addNewSmp(smpId, smpPath);
    }
}

bool hasUpdatedBampLib()
{
    for(map<unsigned, SampleInfoStruct>::iterator it = g_mBampSamples.begin(); it != g_mBampSamples.end(); it ++){
        if(it->second.bUpdated) return true;
    }
    return false;
}


void *buildIndexProcess(void * param)
{
    unsigned libnum  = g_mBampSamples.size();
    assert(libnum != 0);
    BAI_InputItem* smpArr = new BAI_InputItem[libnum];
    size_t procCnt = 0;

    for(map<unsigned, SampleInfoStruct>::iterator it=g_mBampSamples.begin(); it != g_mBampSamples.end(); it++){
        unsigned len;
        float fsecs;
        char *data = reinterpret_cast<char*>(WavExtractToBufferG(it->second.fname.c_str(), 0, 0, 0, -1, &len, &fsecs));
        if(!data){
            BLOGE("buildIndexProcess fail to read sample from file %s", it->second.fname.c_str());
            continue;
        }
        smpArr[procCnt].iAudioID = it->first;
        strcpy(smpArr[procCnt].acAudioUrl, it->second.fname.c_str());
        smpArr[procCnt].pcDataBuffer = data;
        smpArr[procCnt].iBufferSize = len;
        smpArr[procCnt].iDataType = 0;
        procCnt ++;
    }
    assert(procCnt > 0);
    
    BLOGD("before extractDNA, pcDataBuffer: %lx, bufLen: %d.", reinterpret_cast<long unsigned >(static_cast<void*>(smpArr[0].pcDataBuffer)), smpArr[0].iBufferSize);
    BAI_ExtractDNA(smpArr, procCnt, 0);
    BLOGD("after extractDNA, pcDataBuffer: %lx, bufLen: %d.", reinterpret_cast<long unsigned>(static_cast<void*>(smpArr[0].pcDataBuffer)), smpArr[0].iBufferSize);
    BAI_BuildIndex(smpArr, procCnt, g_szBampLibFile, 1);

    struct stat statbuf;
    assert(stat(g_szBampLibFile, &statbuf) == 0);
    return NULL;
}

static char g_szBampSampleListFile[MAX_PATH] = "ioacas/BampSampleList";
static time_t g_TaskLastWatch = 0;
static char g_szBampTaskFile[MAX_PATH] = "ioacas/BampTask";
/**
 * update bamp library by building indexing from local samples or just loading new index.
 * TODO the offline flow needs reconsideration.
 */
 #if 0
void *OnecircleOffline(void *param)
{
    if(!bamp_loadLocalLib())
    {
        time_t mtime = 0;
        struct stat statbuf;
        if(stat(g_szBampSampleListFile, &statbuf) == 0){
            mtime = statbuf.st_mtime;
            if(mtime != g_TaskLastWatch){
                updateBampLib(g_szBampSampleListFile);
                if(hasUpdatedBampLib()){
                   buildIndexProcess(NULL); 
                }
                g_TaskLastWatch = mtime;
            }
        }
    }
    else{
        
    }
    
    return NULL;
}
#endif

//msg return data follow write needs, length of which is stored in outlen.
bool parseModelInfo(char *msg, unsigned & outlen)
{
    bool ret = true;
    string strMsg;
    strMsg.resize(outlen);
    memcpy(const_cast<char*>(strMsg.c_str()), msg, outlen);

    ModelInfo model;
    if(!model.ParseFromString(strMsg)){
        BLOGE("fail to parse modelinfo from binary data.");
        ret = false;
    }
    string taskId = model.taskid();
    string modelUrl = model.modelurl();
    BLOGI("task: Id=%s; Url=%s", taskId.c_str(), modelUrl.c_str());
    //load bamp library.
    LoadResult result;
    result.set_taskid(taskId);
    
    if(!bamp_loadNewLib(modelUrl.c_str())){
        result.set_status(LoadResult::FALURE);
    }
    else{
        result.set_status(LoadResult::SUCCESS);
        if(!moveFile(modelUrl.c_str(), g_szBampLibFile)){
            BLOGE("failed to move new lib to libdir for persistent use. strerr: %s", strerror(errno));
        }

        //if(!copyFile_S(modelUrl.c_str(), g_szBampLibFile)){
        //    BLOGE("failed to copy new lib to subdir for persistent use. strerr: %s", strerror(errno));
        //}
        //else{
        //    remove(modelUrl.c_str());
        //}
    }
    BLOGI("loadresult: %s",result.ShortDebugString().c_str());
    ostringstream oss;
    result.SerializeToOstream(&oss);
    outlen = oss.str().size();
    memcpy(msg, oss.str().c_str(), outlen);
    return ret;
}


bool readMsg(int clifd, char *buf, unsigned &len)
{
    bool ret = true;
    errno = 0;
    int rret = read(clifd, buf, len);
    if(rret == 0){
        BLOGE("clifd read eof.");
        ret = false;
    }
    else if(rret < 0){
        BLOGE("error occure while read from clifd. err: %s.", strerror(errno));
        ret = false;
    }
    assert(len == rret);
    len = rret;
    buf[len] = '\0';
    return ret;
}

bool writeMsg(int clifd, const char* buf, unsigned len)
{
    bool ret = true;
    errno = 0;
    int wret = write(clifd, buf, len);
    if(wret < 0){
        BLOGE("error occure while write to clifd. err: %s.", strerror(errno));
        ret = false;
    }
    assert(wret == len);
    return ret;
}
bool procModelFromClient(int clifd)
{
    bool ret = false;
    while(true){
        char tmpBuf[MAX_BUF];
        unsigned bufLen = 5;
        if(!readMsg(clifd, tmpBuf, bufLen)) break;
        TaskHeader thd;
        string strBuf;
        strBuf.resize(5);
        memcpy(const_cast<char*>(strBuf.c_str()), tmpBuf, 5);
        if(!thd.ParseFromString(strBuf)){
            BLOGE("fail to parse taskheader.");
            break;
        }
        bufLen = thd.tasklength();
        if(!readMsg(clifd, tmpBuf, bufLen)) break;

        parseModelInfo(tmpBuf, bufLen);
        ResultHeader rhd;
        rhd.set_resultlength(bufLen);
        string strRhd;
        rhd.SerializeToString(&strRhd);
        const char *szRhd = strRhd.c_str();
        unsigned szRhdLen = strRhd.size();
        if(!writeMsg(clifd, szRhd, szRhdLen)) break;
        if(!writeMsg(clifd, tmpBuf, bufLen)) break;
        ret = true;
        break;
    }
    close(clifd);
    return ret;
}

int initTcpServer(const char* szIp, unsigned short port)
{
    struct sockaddr_in addr;
    addr = str2addr(szIp, port);
    errno = 0;
    int servfd = initserver(SOCK_STREAM, reinterpret_cast<sockaddr*>(&addr), sizeof(addr), 5);
    if(servfd  <= 0){
        BLOGE("fail to initserver, err: %d, strerr: %s.", errno, strerror(errno));
        exit(1);
    }
    return servfd;
}

void * UpdateLibraryThread(void *param)
{
    char szIp[50];
    if(g_szBampIp[0] == '\0'){
        strcpy(szIp, "192.168.10.95");
    }
    else{
        strncpy(szIp, g_szBampIp, 50);
    }
    BLOGI("bamp server --- %s:%u", szIp, g_uBampPort);
    int servfd = initTcpServer(g_szBampIp, g_uBampPort);
    struct pollfd fdarr[1];
    fdarr[0].fd = servfd;
    fdarr[0].events = POLLIN;
    int err = 0;
    int waitsecs = 5;
    while((err = poll(fdarr, 1, waitsecs * 1000)) >=0){
        if(err == 0){
            //do some cyclic jobs.
            //OnecircleOffline(NULL);

            continue;
        }
        if(fdarr[0].revents & POLLIN){
            struct sockaddr_in addr;
            socklen_t len = sizeof(addr);
            int clifd = accept(servfd, reinterpret_cast<struct sockaddr*>(&addr), &len);
            if(clifd < 0){
                BLOGE("accept error: %s.", strerror(errno));
                continue;
            }
            BLOGI( "client --- %s.", addr2str(reinterpret_cast<sockaddr*>(&addr)).c_str());
            procModelFromClient(clifd);
        }

        if(fdarr[0].revents & POLLERR){
            BLOGE("(POLLERR) an error has occured.");
            break;
        }
        if(fdarr[0].revents & POLLHUP){
            BLOGE("(POLLHUP) a hangup has occured.");
            break;
        }
        if(fdarr[0].revents & POLLNVAL){
            BLOGE("(POLLNVAL) the descriptor does not reference an open file.");
            break;
        }
    }
    if(err == -1){
        BLOGE("error occured while polling sockfd, err: %s", strerror(errno));
    }
    close(servfd);
    return NULL;
}

