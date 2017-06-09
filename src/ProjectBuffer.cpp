/*************************************************************************
  > File Name: ProjectBuffer.cpp
  > Author: zhengshurui
  > Mail:  zhengshurui@thinkit.cn
  > Created Time: Mon 12 Sep 2016 12:59:18 AM PDT
 ************************************************************************/
#include "ProjectBuffer.h"

#include <unistd.h>
#include <pthread.h>
#include <sys/time.h>

#include <cassert>
#include <vector>
#include <map>
#include <set>
#include <list>
#include<iostream>
#include <sstream>
#include <algorithm>
#include <iterator>

#include "utilites.h"
#include "log4z.h"
extern zsummer::log4z::ILog4zManager *g_Log4zManager;

using namespace std;

#define PCM_PERSEC_SMPS 8000
#define PCM_PERSEC_LEN 16000
namespace zen4audio{

static unsigned BLOCKSIZE= 45 * 16000;
static unsigned g_uBlocksMin = 300; //reserved for potential use.
static unsigned g_uBlocksMax = 600;

static list<DataBlock> g_liFreeBlocks;
static set<DataBlock> g_liUsedBlocks;
//static LockHelper g_BlockManaLocker;
static inline DataBlock BlockMana_alloc()
{
    //AutoLock lock(g_BlockManaLocker);
    list<DataBlock>::iterator it = g_liFreeBlocks.begin();
    for(; it != g_liFreeBlocks.end(); it++){
        if(it->getPeerNum() == 1) break;
    }
    if(it != g_liFreeBlocks.end()){
        pair<set<DataBlock>::iterator, bool> reti = g_liUsedBlocks.insert(*it);
        assert(reti.second);
        g_liFreeBlocks.erase(it);
        assert((*reti.first).offset == 0 && (*reti.first).len == 0);
        return *reti.first;
    }
    else{
        size_t allcnt = g_liFreeBlocks.size() + g_liUsedBlocks.size();
        if(allcnt > g_uBlocksMax) return DataBlock();
        else {
            pair<set<DataBlock>::iterator, bool> reti = g_liUsedBlocks.insert(DataBlock(BLOCKSIZE));
            assert(reti.second);
            assert((*reti.first).offset == 0 && (*reti.first).len == 0);
            return *reti.first;
        }
    }
}

static inline void BlockMana_relse(const DataBlock& blk)
{
    //AutoLock lock(g_BlockManaLocker);
    assert(blk.offset + blk.len <= blk.getCap());
    assert(g_liUsedBlocks.find(blk) != g_liUsedBlocks.end());
    if(g_liFreeBlocks.size() + g_liUsedBlocks.size() <= g_uBlocksMin){
        g_liFreeBlocks.push_back(blk);
        g_liFreeBlocks.back().len = 0;
        g_liFreeBlocks.back().offset = 0;
        g_liUsedBlocks.erase(blk);
    }
    else{
        g_liUsedBlocks.erase(blk);
    }
}

struct timeval ZERO_TIMEVAL;
static LoggerId g_BufferLogger = LOG4Z_MAIN_LOGGER_ID;
ProjectBuffer::BufferConfig ProjectBuffer::bufferConfig;
unsigned ProjectBuffer::ceilUnitIdx;
unsigned ProjectBuffer::ceilOffset;

BufferConfig::BufferConfig():
    ProjectBuffer::BufferConfig(), m_uBlockSize(BLOCKSIZE), m_uBlocksMin(g_uBlocksMin), m_uBlocksMax(g_uBlocksMax)
{ }
bool ProjectBuffer::initGlobal(ProjectBuffer::BufferConfig param)
{
    bufferConfig = param;
    ceilUnitIdx = param.waitLength / BLOCKSIZE;
    ceilOffset = param.waitLength - ceilUnitIdx * BLOCKSIZE;
    if(ceilOffset == 0 && ceilUnitIdx != 0){
        ceilOffset = BLOCKSIZE;
        ceilUnitIdx --;
    }
    ceilUnitIdx += 1;//the first element of arrUnits is just placeholder.
    return true;
}

void ProjectBuffer::setPid(unsigned long pid, time_t curTime, bool bbamp)
{
    AutoLock myLock(m_BufferLock);
    if(curTime == 0) curTime = time(NULL);
    this->ID = pid;
    this->bFull = false;
    this->bAlloc = true;
    this->bHasBamp = bbamp;
    this->bBampHit = false;
    this->bRelsed = false;
    this->bampEndIdx = 0;
    this->bampEndOffset = BLOCKSIZE;
    this->mainRegStTime.tv_sec = 0;
    this->mainRegStTime.tv_usec = 0;
    this->mainRegEdTime.tv_sec = 0;
    this->mainRegEdTime.tv_usec = 0;
    DataBlock ele;
    ele.len = BLOCKSIZE;
    arrUnits.push_back(ele);
    prjTime = curTime;
}

/**
 * return array of Objects which share the same point with DataBlock.
 */
void ProjectBuffer::getData(vector<DataBlock>& vec)
{
    AutoLock lock(m_BufferLock);
    vector<DataBlock>::iterator it = arrUnits.begin();
    vec.insert(vec.end(), ++it, arrUnits.end());
    return;
}

unsigned ProjectBuffer::getDataLength()
{
    AutoLock lock(m_BufferLock);
    vector<DataBlock>::iterator it = arrUnits.begin();
    unsigned ret = 0;
    while(++it != arrUnits.end()){
        ret += it->len;
    }
    return 0;
}

void ProjectBuffer::getDataSegmentIn(unsigned idx, unsigned offset, unsigned endIdx, unsigned endOffset, vector<DataBlock>& data)
{
    while(idx < endIdx){
        assert(offset <= arrUnits[idx].len);
        unsigned len = arrUnits[idx].len - offset;
        if(len > 0){
            data.push_back(arrUnits[idx]);   
            data.back().offset = offset;
            data.back().len = len;
        }
        idx ++;
        offset = arrUnits[idx].offset;
    }
    assert(offset <= endOffset);
    if(offset < endOffset){
        unsigned len = endOffset- offset;
        data.push_back(arrUnits[idx]);
        data.back().offset = offset;
        data.back().len = len;
    }
}

void ProjectBuffer::getUnBampData(unsigned &preidx, unsigned &prest, unsigned &endidx, unsigned &endst, std::vector<DataBlock>& data, bool& bPreHit, bool& bfull, struct timeval &prjtime)
{
    AutoLock lock(m_BufferLock);
    data.clear();
    preidx = this->bampEndIdx;
    prest = this->bampEndOffset;
    endidx = arrUnits.size() - 1;
    endst = arrUnits[endidx].len;
    assert(endidx >= preidx);
    unsigned idx = preidx;
    unsigned st = prest;
    getDataSegmentIn(idx, st, endidx, endst, data);
    bPreHit = this->bBampHit;
    bfull = this->bFull;
    prjtime.tv_sec = this->prjTime;
    prjtime.tv_usec = 0;
}

ostream& operator<<(ostream& str, ProjectBuffer::ArrivalRecord rec)
{
    str<<"(" << rec.seconds<< ", " << rec.unitIdx<< ", "<< rec.offset<<")";
    return str;
}

void ProjectBuffer::relse(vector<DataBlock>& retArr){
    retArr.clear();
    AutoLock lock(m_BufferLock);
    if(bRelsed) return;
    bRelsed = true;

    const unsigned addInfoLen = 50;
    char szAddInfo[addInfoLen];
    szAddInfo[0] = '\0';
    if(bHasBamp){
        unsigned unbampBytes = (arrUnits.size() - 1 - this->bampEndIdx) * BLOCKSIZE + arrUnits.back().len + arrUnits.back().offset - this->bampEndOffset;
        if(unbampBytes > 0){
            snprintf(szAddInfo, addInfoLen, "unbampBytes: %u", unbampBytes);
        }
    }

    unsigned tolLen = 0;
    for(size_t idx=1; idx < arrUnits.size(); idx++){
        tolLen += arrUnits[idx].len;
    }
    retArr.insert(retArr.end(), arrUnits.begin() + 1, arrUnits.end());
    arrUnits.resize(1);
    LOGFMT_INFO(g_BufferLogger, "PID=%lu SIZE=%lu release ProjectBuffer. %s", this->ID, tolLen, szAddInfo);
    LOGFMT_DEBUG(g_BufferLogger, "PID=%lu MainReg start %ld %ld; end: %ld %ld.", this->ID, mainRegStTime.tv_sec, mainRegStTime.tv_usec, mainRegEdTime.tv_sec, mainRegEdTime.tv_usec);
}

/**
 * 返回成功接收的长度.
 *
 */
unsigned ProjectBuffer::recvData(char *data, unsigned dataLen, DataBlock* blk, time_t curTime)
{
    AutoLock lock(m_BufferLock);
    unsigned ret = 0;
    if(curTime == 0) curTime = time(NULL);
    unsigned curSize = arrUnits.back().len;
    unsigned size1 = dataLen > BLOCKSIZE - curSize ? BLOCKSIZE - curSize : dataLen;
    unsigned size2 = dataLen - size1;
    memcpy(arrUnits.back().getPtr() + curSize, data, size1);
    ret += size1;
    arrUnits.back().len += size1;
    if(size2 > 0){
        assert(blk->getPtr());
        arrUnits.push_back(*blk);
        memcpy(arrUnits.back().getPtr(), data + size1, size2);
        ret += size2;
        arrUnits.back().len += size2;
    }
    lastRecord.seconds = curTime;
    lastRecord.unitIdx = arrUnits.size() - 1;
    lastRecord.offset = arrUnits.back().len;
    if(!bFull && ret > 0){
        if(arrUnits.size() - 1 > ceilUnitIdx || (arrUnits.size() -1 == ceilUnitIdx && arrUnits.back().len >= ceilOffset)){
            LOGFMT_DEBUG(g_BufferLogger, "PID=%lu the state turns to FULL, as the accumulated data reaches to maximum.", this->ID);
            turnFull();
        }
    }
    setReflesh_un();
    return ret;
}

void ProjectBuffer::finishRecv()
{
    AutoLock lock(m_BufferLock);
    LOGFMT_DEBUG(g_BufferLogger, "PID=%lu the state turns to FULL, as being notified.", this->ID);
    if(! bFull){
         turnFull();   
        setReflesh_un();
    }
}

bool ProjectBuffer::isFull(time_t curTime, time_t& nextTime)
{
    AutoLock lock(m_BufferLock);
    if(bFull){
        return true;
    }
    if(!bAlloc) return false;
    bool ret = false;
    if(curTime == 0) curTime = time(NULL);
    ArrivalRecord &edRec = lastRecord;
    //ArrivalRecord &stRec = lastRecord;
    unsigned long diffTime = curTime > edRec.seconds ? curTime - edRec.seconds : 0;
    unsigned long totalTime = curTime > prjTime ? curTime - prjTime : 0;
    if(diffTime > bufferConfig.waitSecondsStep){
        turnFull();
        setReflesh_un();
        LOGFMT_DEBUG(g_BufferLogger, "PID=%lu the state turns to FULL, diffTime: %lu.", this->ID, diffTime);
        ret = true;
    }
    else if(totalTime > bufferConfig.waitSeconds){
        turnFull();
        setReflesh_un();
        LOGFMT_DEBUG(g_BufferLogger, "PID=%lu the state turns to FULL, totalTime: %lu.", this->ID, totalTime);
        ret = true;
    }
    nextTime = edRec.seconds + bufferConfig.waitSecondsStep;
    return ret;
}


int ProjectBuffer::subscribeReflesh()
{
    AutoLock lock(m_BufferLock);
    int ret = -1;
    for(int idx=0; idx< allMarksCapa; idx++){
        if(validMarkIdc.find(idx) == validMarkIdc.end()){
            ret = idx;
            assert(allMarks[ret] == NULL);
            allMarks[ret] = new DataRefleshMark();
            break;
        }
    }
    return ret;
}
void ProjectBuffer::unsubscribeReflesh(int hdl)
{
    AutoLock lock(m_BufferLock);
    assert(validMarkIdc.erase(hdl) == 1);
    delete allMarks[hdl];
    allMarks[hdl] = NULL;
}
void ProjectBuffer::setReflesh_un()
{
    for(set<int>::iterator it=validMarkIdc.begin(); it != validMarkIdc.end(); it++){
        DataRefleshMark *curptr = allMarks[*it];
        pthread_mutex_lock(&curptr->lock);
        curptr->bReflesh = true;
        pthread_mutex_unlock(&curptr->lock);
    }
}

//////////////////////////////////////////////////////////
//
struct ProjectCheckbook{
    ProjectCheckbook():
        prj(NULL), cnt(0), leftSize(0)
    {}
    ProjectBuffer *prj;
    int cnt;
    unsigned leftSize;
};

struct ProjectFullInfo{
    explicit ProjectFullInfo(ProjectBuffer *p=NULL, time_t n=0):
        prj(p), nextTime(n)
    {}
    ProjectBuffer *prj;
    time_t nextTime;
};

static std::map<unsigned long, ProjectCheckbook> g_mapProjs;
//static std::set<unsigned long> g_setPrevFullIDs;
static map<unsigned long, ProjectFullInfo> g_mapPrevFullIDs;
static std::list<ProjectFullInfo> g_liFullIDs;
//static std::set<unsigned long> g_setPostFullIDs;
static LockHelper g_ProjsLocker;

static LockHelper g_PrevFullIDsLocker;
static LockHelper g_ProjsIDsLocker;
//static LockHelper g_PostFullIDsLocker;
static pthread_cond_t g_ProjsFullCond;
//static pthread_cond_t g_ProjsEmptyCond;
static FuncPushProj g_PushProjAddr;

void getBufferStatus(string &outstr)
{
    outstr.resize(1024);
    char *tmpSt = const_cast<char*>(outstr.c_str());
    unsigned curlen = 0;
    curlen += sprintf(tmpSt + curlen, "BlockSize=%u BlocksMax=%u BlocksMin=%u\n", BLOCKSIZE, g_uBlocksMax, g_uBlocksMin);
    g_ProjsLocker.lock();
    curlen += sprintf(tmpSt + curlen, "FreeBlocks: %lu    UsedBlocks: %lu\n", g_liFreeBlocks.size(), g_liUsedBlocks.size());
    curlen += sprintf(tmpSt + curlen, "allProjsIDs: %lu    ", g_mapProjs.size());
    g_ProjsLocker.unLock();

    g_PrevFullIDsLocker.lock();
    curlen += sprintf(tmpSt + curlen, "PrevFullIDs: %lu    ", g_mapPrevFullIDs.size());
    g_PrevFullIDsLocker.unLock();
    g_ProjsIDsLocker.lock();
    curlen += sprintf(tmpSt + curlen, "FullIDs: %lu\n", g_liFullIDs.size());
    g_ProjsIDsLocker.unLock();

}

static void makeTimespec(struct timespec *tsp)
{
    struct timeval now;
    gettimeofday(&now, NULL);
    tsp->tv_sec = now.tv_sec;
    tsp->tv_nsec = now.tv_usec * 1000;
}

bool init_bufferglobal(BufferConfig buffConfig, FuncPushProj pushProjAddr)
{
    AutoLock myLock(g_ProjsLocker);
    ProjectBuffer::initGlobal(static_cast<ProjectBuffer::BufferConfig>(buffConfig));
    BLOCKSIZE=buffConfig.m_uBlockSize;
    g_uBlocksMax = buffConfig.m_uBlocksMax;
    g_uBlocksMin = buffConfig.m_uBlocksMin;
    g_BufferLogger = g_Log4zManager->createLogger("globalBuffer");
    LOGFMT_INFO(g_BufferLogger, "init_bufferglobal finished.");
    LOGFMT_INFO(g_BufferLogger, "blocksMaximum=%u.", g_uBlocksMax);
    LOGFMT_INFO(g_BufferLogger, "blocksMiniMum=%u.", g_uBlocksMin);
    LOGFMT_INFO(g_BufferLogger, "blockSize=%u.", BLOCKSIZE);
    LOGFMT_INFO(g_BufferLogger, "waitSecondsStep=%u.", ProjectBuffer::bufferConfig.waitSecondsStep);
    LOGFMT_INFO(g_BufferLogger, "waitSeconds=%u.", ProjectBuffer::bufferConfig.waitSeconds);
    LOGFMT_INFO(g_BufferLogger, "waitLength=%u.", ProjectBuffer::bufferConfig.waitLength);
    g_PushProjAddr = pushProjAddr;
    return true;
}

void rlse_bufferglobal()
{
    AutoLock myLock(g_ProjsLocker);

    g_mapProjs.clear();
    g_liFullIDs.clear();
    //g_setPostFullIDs.clear();
    LOGFMT_DEBUG(g_BufferLogger, "rlse_bufferglobal finished.");
}

void notifyProjFinish(unsigned long pid)
{
    //from prevfullIds to lifullIds.
    ProjectFullInfo prjInfo;
    g_PrevFullIDsLocker.lock();
    if(g_mapPrevFullIDs.find(pid) == g_mapPrevFullIDs.end()){
        g_PrevFullIDsLocker.unLock();
        return;
    }
    prjInfo = g_mapPrevFullIDs[pid];
    g_mapPrevFullIDs.erase(pid);
    g_PrevFullIDsLocker.unLock();

    prjInfo.prj->finishRecv();

    g_ProjsIDsLocker.lock();
    g_liFullIDs.push_back(prjInfo);
    g_ProjsIDsLocker.unLock();
    pthread_cond_signal(&g_ProjsFullCond);
    LOGFMT_DEBUG(g_BufferLogger, "notifyProjFinish PID=%lu.", pid);
}

/**
 *
 */
void recvProjSegment(ProjectSegment param, bool iswait)
{
    //LOGFMT_TRACE(g_BufferLogger, "PID=%llu receive new data, size=%u.", pid, len);
    unsigned long &pid = param.pid;
    char *&data = param.data;
    unsigned dataLen = param.len;

    ProjectBuffer *tmpPrj = NULL;
    g_ProjsLocker.lock();
    if(g_mapProjs.find(pid) == g_mapProjs.end()){
        time_t curTime;
        time(&curTime);
        ProjectBuffer * tmpPrj = new ProjectBuffer(pid, curTime, g_PushProjAddr != NULL);
        g_mapProjs[pid].prj = tmpPrj;
        //if(bBamp) g_mapProjs[pid].prj->setBamp();
        if(g_PushProjAddr != NULL){
            int refnum = g_PushProjAddr(tmpPrj);
            if(refnum > 0) g_mapProjs[pid].cnt += refnum;
        }
        //add to setPrevFullIDs.
        g_PrevFullIDsLocker.lock();
        g_mapPrevFullIDs.insert(std::make_pair(pid, ProjectFullInfo(tmpPrj, curTime)));
        g_PrevFullIDsLocker.unLock();
        g_mapProjs[pid].cnt ++;
        LOGFMT_DEBUG(g_BufferLogger, "PID=%lu arrives, %u projects in buffer.", pid, g_mapProjs.size());
    }
    tmpPrj = g_mapProjs[pid].prj;
    g_mapProjs[pid].cnt ++;
    DataBlock blk;
    bool bvalidblk = false;
    if(g_mapProjs[pid].leftSize < dataLen){
        while(true){
            blk = BlockMana_alloc();
            if(blk.getPtr()){
                g_mapProjs[pid].leftSize = BLOCKSIZE - dataLen + g_mapProjs[pid].leftSize;
                bvalidblk = true;
                break;
            }
            else if(!iswait){
                LOGFMT_ERROR(g_BufferLogger, "PID=%lu GlobalBuffer receive data failed. as DataBlock pool is full.", pid);
                break;
            }
            else{
                g_ProjsLocker.unLock();
                usleep(100000);
                g_ProjsLocker.lock();
            }
        }
    }
    else{
        g_mapProjs[pid].leftSize -= dataLen;
    }
    g_ProjsLocker.unLock();

    if(bvalidblk)
        unsigned rlen = tmpPrj->recvData(data, dataLen, &blk);
    /*
       if(rlen == dataLen){
       } 
       else{
       LOGFMT_ERROR(g_BufferLogger, "PID=%lu GlobalBuffer receive data failed. Total: %u; Received: %u.", pid, dataLen, rlen);
       }
       */
    returnBuffer(tmpPrj);
}

void obtainAllBuffers(map<unsigned long, ProjectBuffer*>& allBufs)
{
    g_ProjsLocker.lock();
    for(map<unsigned long, ProjectCheckbook>::iterator it = g_mapProjs.begin(); it != g_mapProjs.end(); it++){
        if(allBufs.find(it->first) == allBufs.end()){
            allBufs[it->first] = it->second.prj;
            it->second.cnt++;
        }
    }
    g_ProjsLocker.unLock();
}

/*
 * TODO reimpliment it, add refcnt only under context of valid projIndex.
 ProjectBuffer* obtainBuffer(unsigned long pid)
 {
 g_ProjsLocker.lock();
 if(g_mapProjs.find(pid) == g_mapProjs.end()){
 g_ProjsLocker.unLock();
 return NULL;
 }
 ProjectBuffer* ret = g_mapProjs[pid].prj;
 g_mapProjs[pid].cnt ++;
 g_ProjsLocker.unLock();
 return ret;
 }
 */

/**
 * from setPrevFullIds to liFullIds.
 */
static void transferIDs(time_t curTime)
{
    g_PrevFullIDsLocker.lock();
    unsigned long curpid = 0;
    while(true){
        time_t *pnextTime;
        map<unsigned long, ProjectFullInfo>::iterator it = g_mapPrevFullIDs.lower_bound(curpid);
        if(it == g_mapPrevFullIDs.end()){
            break;
        }
        curpid = it->first;
        pnextTime = &(it->second.nextTime);
        if(curTime > *pnextTime){
            ProjectBuffer* tmpPrj = it->second.prj;
            g_PrevFullIDsLocker.unLock();
            bool bfull = tmpPrj->isFull(curTime, *pnextTime);
            g_PrevFullIDsLocker.lock();
            if(bfull){
                g_liFullIDs.push_back(g_mapPrevFullIDs[curpid]);
                g_mapPrevFullIDs.erase(curpid);
            }
        }
        curpid ++;
    }
    g_PrevFullIDsLocker.unLock();
}

static void unlockLocker(LockHelper *locker)
{
    locker->unLock();
}
static LockHelper g_ObtainFullLocker;
/**
 * 当存在多个project时，保证多次调用会返回所有的project.
 *
 */
ProjectBuffer* obtainFullBufferTimeout(unsigned secs)
{
    ProjectBuffer *ret = NULL;
    g_ObtainFullLocker.lock();
    pthread_cleanup_push(reinterpret_cast<void(*)(void*)>(unlockLocker), &g_ObtainFullLocker);
    pthread_testcancel();
    g_ProjsIDsLocker.lock();
    time_t endTime = 0;
    while(g_liFullIDs.size() == 0){
        time_t curTime = time(NULL);
        if(endTime == 0){
            endTime = curTime + secs;
            if(endTime < curTime){
                endTime = curTime  + 3600 * 24;
            }
        }
        transferIDs(curTime);
        if(curTime >= endTime) break;
        struct timespec tsp;
        makeTimespec(&tsp);
        tsp.tv_sec += 1;//one circle per second.
        pthread_cond_timedwait(&g_ProjsFullCond, &g_ProjsIDsLocker._crit, &tsp);
    }

    if(g_liFullIDs.size() > 0){
        //transfer the returned from liFullIds to postFullIds.
        ret = g_liFullIDs.front().prj;
        g_liFullIDs.pop_front();
    }
    g_ProjsIDsLocker.unLock();
    pthread_cleanup_pop(1);

    /*
       g_PostFullIDsLocker.lock();
       g_setPostFullIDs.insert(retpid);
       g_PostFullIDsLocker.unLock();
       */

    return ret;
}

void returnBuffer(ProjectBuffer* obtained)
{
    g_ProjsLocker.lock();
    unsigned long pid = obtained->ID;
    assert(g_mapProjs.find(pid) != g_mapProjs.end());
    assert(g_mapProjs[pid].prj == obtained);
    assert(g_mapProjs[pid].cnt > 0);
    g_mapProjs[pid].cnt --;

    if(g_mapProjs[pid].cnt != 0){
        g_ProjsLocker.unLock();
        return; 
    }

    //From setPostFullIDs to vanish(delete from g_mapProjs).
    bool removed = true;
    /*
       g_PostFullIDsLocker.lock();
       if(g_setPostFullIDs.find(pid) != g_setPostFullIDs.end()){
       g_setPostFullIDs.erase(pid);
       removed = true;
       }
       g_PostFullIDsLocker.unLock();
       */
    if(removed){
        vector<DataBlock> retArr;
        obtained->relse(retArr);
        for(size_t idx=0; idx < retArr.size(); idx++){
            BlockMana_relse(retArr[idx]);
        }
        g_mapProjs.erase(obtained->ID);
        delete obtained;
    }
    g_ProjsLocker.unLock();
}

bool isAllFinished()
{
    AutoLock mylock(g_ProjsLocker);
    return g_mapProjs.empty();
}

}
