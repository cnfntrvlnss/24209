/*************************************************************************
    > File Name: ProjectBuffer.h
    > Author: zhengshurui
    > Mail:  zhengshurui@thinkit.cn
    > Created Time: Mon 12 Sep 2016 02:22:16 AM PDT
 ************************************************************************/

#ifndef PROJECTBUFFER__H
#define PROJECTBUFFER__H

#include "utilites.h"
#include <sys/time.h>
#include <pthread.h>
#include <climits>
#include <cstdlib>
#include <cstring>
#include <vector>
#include <list>
#include <set>
#include <ostream>

typedef bool(*FuncSaveData)(FILE*, std::vector<DataBlock>&);
namespace zen4audio{

/**
 * 管理一批DataBlock集合, DataBlock 代表固定长度的一段内存，支持拷贝，赋值， 自动销毁。
 *
 */
extern unsigned BLOCKSIZE;
extern unsigned g_uBlocksMin; //reserved for potential use.
extern unsigned g_uBlocksMax;

extern std::list<DataBlock> g_liFreeDataBlocks;
extern std::set<DataBlock> g_liUsedDataBlocks;
extern pthread_mutex_t g_DataBlocksLock;
static inline DataBlock BlockMana_alloc()
{
    pthread_mutex_lock(&g_DataBlocksLock);
    std::list<DataBlock>::iterator it = g_liFreeDataBlocks.begin();
    for(; it != g_liFreeDataBlocks.end(); it++){
        if(it->getPeerNum() == 1) break;
    }
    if(it != g_liFreeDataBlocks.end()){
        std::pair<std::set<DataBlock>::iterator, bool> reti = g_liUsedDataBlocks.insert(*it);
        assert(reti.second);
        g_liFreeDataBlocks.erase(it);
        assert((*reti.first).offset == 0 && (*reti.first).len == 0);
        pthread_mutex_unlock(&g_DataBlocksLock);
        return *reti.first;
    }
    else{
        size_t allcnt = g_liFreeDataBlocks.size() + g_liUsedDataBlocks.size();
        if(allcnt > g_uBlocksMax){
            pthread_mutex_unlock(&g_DataBlocksLock);
            return DataBlock();
        }
        else {
            std::pair<std::set<DataBlock>::iterator, bool> reti = g_liUsedDataBlocks.insert(DataBlock(BLOCKSIZE));
            assert(reti.second);
            assert((*reti.first).offset == 0 && (*reti.first).len == 0);
            pthread_mutex_unlock(&g_DataBlocksLock);
            return *reti.first;
        }
    }
}

static inline void BlockMana_relse(const DataBlock& blk)
{
    pthread_mutex_lock(&g_DataBlocksLock);
    assert(blk.offset + blk.len <= blk.getCap());
    assert(g_liUsedDataBlocks.find(blk) != g_liUsedDataBlocks.end());
    if(g_liFreeDataBlocks.size() + g_liUsedDataBlocks.size() <= g_uBlocksMin){
        g_liFreeDataBlocks.push_back(blk);
        g_liFreeDataBlocks.back().len = 0;
        g_liFreeDataBlocks.back().offset = 0;
        g_liUsedDataBlocks.erase(blk);
    }
    else{
        g_liUsedDataBlocks.erase(blk);
    }
    pthread_mutex_unlock(&g_DataBlocksLock);
}


extern struct timeval ZERO_TIMEVAL;
/**
 *  用于节目缓存的类, 比之前的实现相比，最小化了功能；用到了唯一的内存管理代码，用于缓存空间的计数与控制.
 *
 *
 */
class ProjectBuffer{
public:
    ProjectBuffer(){
        init();
        for(unsigned idx=0; idx < allMarksCapa; idx++){
            allMarks[idx] = NULL;
        }
    }
    explicit ProjectBuffer(unsigned long pid, time_t curTime = 0, bool bbamp = false){
        init();
        for(unsigned idx=0; idx < allMarksCapa; idx++){
            allMarks[idx] = NULL;
        }
        setPid(pid, curTime, bbamp);
    }
    ~ProjectBuffer(){
        std::vector<DataBlock> retArr;
        relse(retArr);
        assert(retArr.size() == 0);
    }
    void setPid(unsigned long, time_t curTime =0, bool bBamp = false);
    struct timeval getPrjTime(){
        AutoLock lock(m_BufferLock);
        struct timeval ret;
        ret.tv_sec = fullRecord.seconds;
        ret.tv_usec = 0;
        return ret;
    }

    void setBampEndPos(unsigned preIdx, unsigned preSt, unsigned idx, unsigned st, bool bhit){
        AutoLock lock(m_BufferLock);
        assert(this->bampEndIdx == preIdx);
        assert(this->bampEndOffset == preSt);
        this->bampEndIdx = idx;
        this->bampEndOffset = st;
        if(bhit) this->bBampHit = true;
    }
    
    void getUnBampData(unsigned &idx, unsigned &st, unsigned &endidx, unsigned &endst, std::vector<DataBlock>& data, bool& bPreHit, bool& bfull, struct timeval & prjtime);

    void startMainReg(struct timeval curtime = ZERO_TIMEVAL){
        //if(memcmp(&curtime, &ZERO_TIMEVAL, sizeof(struct timeval) == 0)){
        if(curtime.tv_sec == ZERO_TIMEVAL.tv_sec && curtime.tv_usec == ZERO_TIMEVAL.tv_usec){
            gettimeofday(&curtime, NULL);
        }
        AutoLock lock(m_BufferLock);
        mainRegStTime = curtime;
    }
    void finishMainReg(struct timeval curtime = ZERO_TIMEVAL){
        //if(memcmp(&curtime, &ZERO_TIMEVAL, sizeof(struct timeval) == 0)){
        if(curtime.tv_sec == ZERO_TIMEVAL.tv_sec && curtime.tv_usec == ZERO_TIMEVAL.tv_usec){
            gettimeofday(&curtime, NULL);
        }
        AutoLock lock(m_BufferLock);
        mainRegEdTime = curtime;
    }
    
    void relse(std::vector<DataBlock>& retArr);
    void getData(std::vector<DataBlock>& vec);
    unsigned getDataLength();
    bool isFull(time_t curTime, time_t& nextTime);
    bool getFull(){
        AutoLock lock(m_BufferLock);
        return bFull;
    }

    void recvData(DataBlock& blk);
    unsigned recvData(char *data, unsigned len, DataBlock* blk, time_t curTime = 0);
    void finishRecv();
    void init(){
        bAlloc = false;
    }
    void getDataSegment(std::vector<DataBlock>& data, unsigned idx, unsigned offset, unsigned endIdx=0, unsigned endOffset=0){
        AutoLock lock(m_BufferLock);
        idx += 1;
        if(endIdx == 0){
            endIdx = this->arrUnits.size() - 1;
            endOffset = this->arrUnits[endIdx].len;
        }
        else{
            endIdx += 1;
        }
        if(endIdx < idx) return;
        getDataSegmentIn(idx, offset, endIdx, endOffset, data);
    }

    int subscribeReflesh();
    void unsubscribeReflesh(int hdl);
    bool checkReflesh(int hdl){
        DataRefleshMark* curptr = allMarks[hdl];
        pthread_mutex_lock(&curptr->lock);
        bool ret = curptr->bReflesh;
        if(ret){
            curptr->bReflesh = false;
        }
        pthread_mutex_unlock(&curptr->lock);
    }

    struct BufferConfig{
        BufferConfig():
            waitSecondsStep(30), waitSeconds(UINT_MAX), waitLength(UINT_MAX)
        {}
        unsigned waitSecondsStep;
        unsigned waitSeconds;
        unsigned waitLength;
    };
    static BufferConfig bufferConfig;
    static bool initGlobal(BufferConfig bufferConfig = BufferConfig());
    unsigned long ID;
private:
    ProjectBuffer(const ProjectBuffer& );
    ProjectBuffer& operator=(const ProjectBuffer&);
    void turnFull()
    {
        assert(!bFull);
        bFull = true;
        fullRecord = lastRecord;
    }
    void getDataSegmentIn(unsigned idx0, unsigned offset0, unsigned idx1, unsigned offset1, std::vector<DataBlock>& data);
    void setReflesh_un();
    struct ArrivalRecord{
        ArrivalRecord():
            seconds(0), unitIdx(0), offset(0)
        {}
        ArrivalRecord(unsigned long secs, unsigned idx, unsigned offset):
            seconds(secs), unitIdx(idx), offset(offset)
        {}
        time_t seconds;
        //unsigned totalLen;
        unsigned unitIdx;
        unsigned offset;
    };
    friend std::ostream& operator<<(std::ostream&, ArrivalRecord);
    LockHelper m_BufferLock;
    std::vector<DataBlock> arrUnits;
    ArrivalRecord fullRecord;
    ArrivalRecord lastRecord;
    time_t prjTime;
    bool bAlloc;
    bool bFull;
    bool bBampHit;
    bool bHasBamp;
    bool bRelsed;
    unsigned bampEndIdx;
    unsigned bampEndOffset;
    unsigned fullUnitIdx;
    unsigned fullOffset;
    struct timeval mainRegStTime;
    struct timeval mainRegEdTime;
    static unsigned ceilUnitIdx;
    static unsigned ceilOffset;

    struct DataRefleshMark{
        DataRefleshMark(){
            bReflesh = true;
            pthread_mutex_init(&lock, NULL);
        }
        ~DataRefleshMark(){
            pthread_mutex_destroy(&lock);
        }

        bool bReflesh;
        pthread_mutex_t lock;
    };
    static const unsigned allMarksCapa = 4;
    DataRefleshMark *allMarks[allMarksCapa];
    std::set<int> validMarkIdc;
};

struct BufferConfig: public ProjectBuffer::BufferConfig
{
    BufferConfig();
    unsigned m_uBlockSize;
    unsigned m_uBlocksMin;
    unsigned m_uBlocksMax;
};
/***************************************section two*********************************************/

struct ProjectSegment{
    ProjectSegment():
        pid(0), data(NULL), len(0)
    {}
    unsigned long pid;
    char *data;
    unsigned len;
};

bool dispatchOneProj(ProjectBuffer* proj);
typedef int (*FuncPushProj)(ProjectBuffer* prj);
void getBufferStatus(std::string &outstr);
bool init_bufferglobal(BufferConfig buffConfig, FuncPushProj pushProjAddr = NULL);
void rlse_bufferglobal();
extern "C" void notifyProjFinish(unsigned long pid);

void recvProjSegment(unsigned long pid, DataBlock &blk);
void recvProjSegment(ProjectSegment param, bool iswait = false);
ProjectBuffer* obtainBuffer(unsigned long pid);
void obtainAllBuffers(std::map<unsigned long, ProjectBuffer*>& allBufs);
ProjectBuffer* obtainFullBufferTimeout(unsigned secs = -1);
void returnBuffer(ProjectBuffer* obtained);
extern "C" bool isAllFinished();

}

#endif

