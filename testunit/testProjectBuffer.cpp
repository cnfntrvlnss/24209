/*************************************************************************
    > File Name: testProjectBuffer.cpp
    > Author: zhengshurui
    > Mail:  zhengshurui@thinkit.cn
    > Created Time: Tue 20 Sep 2016 07:17:34 PM PDT
 ************************************************************************/

#include <cstdio>
#include <cstdlib>
#include <cassert>
#include<iostream>
#include <string>
#include <vector>

#include "../src/log4z.h"
//#include "../src/waveinfo.h"
#include "../src/ProjectBuffer.h"
#include "../src/utilites.h"

#include "gtest/gtest.h"

using namespace std;
using namespace zen4audio;
#define MAX_PATH 512
char workDir[MAX_PATH];

struct WavBuf{
    WavBuf():
        buf(NULL), len(0)
    {}
    char *buf;
    unsigned len;
};
unsigned incrWavBuf(WavBuf& buf)
{
    unsigned addLen = rand() % 60 * 16000;
    buf.buf = (char*)realloc(buf.buf, buf.len + addLen);
    assert(buf.buf != NULL);
    unsigned leftSize = addLen;
    char *untagPtr = buf.buf + buf.len;
    int randVal;
    while(leftSize >= sizeof(int)){
        randVal = rand();
        memcpy(untagPtr, &randVal, sizeof(int));
        untagPtr += sizeof(int);
        leftSize -= sizeof(int);
    }
    char randCh;
    while(leftSize > 0){
        randCh = (char)rand();
        memcpy(untagPtr, &randCh, 1);
        untagPtr += 1;
        leftSize -= 1;
    }
    
    unsigned lastLen = buf.len;
    buf.len += addLen;
    return lastLen;
}
#if 0
WavBuf loadWavBuf(const char* filePath)
{
    WavBuf ret;
    const unsigned stepSize = 16000 * 60 * 3;
    FILE *fp = fopen(filePath, "rb");
    assert(fp != NULL);
    fseek(fp, 0, SEEK_END);
    fseek(fp, sizeof(PCM_HEADER), SEEK_SET);
    while(true){
        ret.buf = (char*)realloc(ret.buf, ret.len + stepSize);
        assert(ret.buf != NULL);
        unsigned addLen = fread(ret.buf + ret.len, 1, stepSize, fp);
        ret.len += addLen;
        if(stepSize != addLen){
            ret.buf = (char*)realloc(ret.buf, ret.len);
            break;
        }
    }
    fclose(fp);
    LOGFMTI("load data from file: %s; size: %u.", filePath, ret.len);
    return ret;
}
#endif

void checkWavEqual(vector<DataBlock> arr, WavBuf buf)
{
    unsigned curSize = 0;
    for(vector<DataBlock>::iterator it= arr.begin(); it != arr.end(); it++){
        EXPECT_EQ(0, memcmp(buf.buf + curSize, it->m_buf, it->m_bufLen));
        curSize += it->m_bufLen;
    }
    EXPECT_EQ(curSize, buf.len);
}

struct ProjSentInfo{
    unsigned long ID;
    WavBuf sentBuf;
};

void checkBuffer(ProjSentInfo curProj)
{
    set<unsigned long> passedIds;
    ProjectBuffer* proj = NULL;
    while(true){
        proj = obtainFullBufferTimeout(0);
        ASSERT_NE((ProjectBuffer*)NULL, proj);
        if(proj->ID == curProj.ID) break;
        EXPECT_EQ(passedIds.end(), passedIds.find(proj->ID));
        if(passedIds.find(proj->ID) != passedIds.end()) break;
        passedIds.insert(proj->ID);
    }
    if(proj->ID == curProj.ID){
        vector<DataBlock> arr;
        proj->getData(arr);
        SCOPED_TRACE("checkBuffer");
        checkWavEqual(arr, curProj.sentBuf);
    }
}

TEST(FirstCase, DISABLED_SerialCopyTest)
{
    BufferConfig config;
    unsigned fullLen = 60* 16000;
    config.waitLength = fullLen;
    config.waitSecondsStep = 3;
    assert(init_bufferglobal(config));
    for(int pid=0; pid < 10; pid++){
        int circleCnt = 10;
        ProjSentInfo projInfo;
        projInfo.ID = pid;
        WavBuf &buf = projInfo.sentBuf;
        while(circleCnt-- > 0){
            unsigned st = incrWavBuf(buf);
            assert(recvProjSegment(pid, buf.buf + st, buf.len - st) == 1);
            if(fullLen > buf.len){
                EXPECT_EQ(NULL, obtainFullBufferTimeout(0));
            }
            else{
                checkBuffer(projInfo);
            }
        }
        notifyProjFinish(pid);
        ProjectBuffer* proj = obtainFullBufferTimeout();
        ASSERT_NE((ProjectBuffer*)NULL, proj);
        vector<DataBlock> dataCopy;
        proj->getData(dataCopy);
        SCOPED_TRACE("serial");
        checkWavEqual(dataCopy, buf);

        returnFullBuffer(proj);
        assert(isAllFinished());
        free(buf.buf);
    }
    
    rlse_bufferglobal();
}


void checkBuffers(vector<ProjSentInfo> arrProjs)
{
    while(!isAllFinished()){
        ProjectBuffer* proj = obtainFullBufferTimeout();
        EXPECT_NE((ProjectBuffer*)NULL, proj);
        vector<DataBlock> arr;
        proj->getData(arr);
        WavBuf buf;
        vector<ProjSentInfo>::iterator it;
        for(it=arrProjs.begin(); it != arrProjs.end(); it++){
            if(it->ID == proj->ID){
                buf = it->sentBuf;
                break;
            } 
        }
        EXPECT_NE(it, arrProjs.end());
        SCOPED_TRACE("summary-parallel");
        checkWavEqual(arr, it->sentBuf);
        returnFullBuffer(proj);
        arrProjs.erase(it);
        
    }
    EXPECT_EQ(0, arrProjs.size());
}

TEST(FirstCase, DISABLED_ParallelCopyTest){
    BufferConfig config;
    unsigned fullLen = 60* 16000;
    config.waitLength = fullLen;
    config.waitSecondsStep = 3;
    assert(init_bufferglobal(config));
    vector<ProjSentInfo> arrProjs;
    for(int i=0; i<10; i++){
        ProjSentInfo proj;
        proj.ID = i;
        arrProjs.push_back(proj);
    }
    int circleCnt = 10;
    while(circleCnt -- > 0){
        for(int i=0; i<10; i++){
            unsigned long pid = arrProjs[i].ID;
            WavBuf &buf = arrProjs[i].sentBuf;
            unsigned st = incrWavBuf(buf);
            EXPECT_EQ(1, recvProjSegment(pid, buf.buf + st, buf.len - st));
            if(fullLen <= buf.len){
                checkBuffer(arrProjs[i]);
            }
        }
    }

    checkBuffers(arrProjs);
    for(int i=0; i<10; i++){
        if(arrProjs[i].sentBuf.buf){
            free(arrProjs[i].sentBuf.buf);
        }
    }
    rlse_bufferglobal();
}


namespace zen4audio{

    extern std::vector<DataUnit*> g_vecFreeBlocks; 
}
TEST(FirstCase, SaturateBufferTest){
    BufferConfig config;
    config.waitLength = -1;
    config.m_uBlocksMin = 10;
    config.m_uBlocksMax = 10;
    config.waitSecondsStep = 5;
    assert(init_bufferglobal(config));

    for(int circnt=0; circnt< 3; circnt++){
        int failcnt = 0;
        while(true){
            for(int pid=0; pid < 20; pid++){
                WavBuf buf;
                incrWavBuf(buf);
                if(recvProjSegment(pid, buf.buf, buf.len) != 1){
                    failcnt ++;
                }
                free(buf.buf);
            }
            if(failcnt > 20) break;
        }
        unsigned blockcnt = 0;
        unsigned pidcnt = 0;
        while(!isAllFinished()){
            pidcnt ++;
            ProjectBuffer *proj = obtainFullBufferTimeout(-1);
            ASSERT_NE((ProjectBuffer*)NULL, proj);
            vector<DataBlock> arr;
            proj->getData(arr);
            blockcnt += arr.size();
            returnFullBuffer(proj);
        }
        ASSERT_EQ(10, blockcnt);
        EXPECT_EQ(20, pidcnt);
        EXPECT_EQ(10, zen4audio::g_vecFreeBlocks.size());

    }
    rlse_bufferglobal();
}
int main(int argc, char** argv)
{
    strncpy(workDir, "temp/", MAX_PATH);
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

