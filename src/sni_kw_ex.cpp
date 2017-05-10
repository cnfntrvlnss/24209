/*************************************************************************
	> File Name: sni_kw_ex.cpp
	> Author: 
	> Mail: 
	> Created Time: Thu 04 May 2017 10:12:49 PM EDT
 ************************************************************************/

#include "sni_kw_ex.h"
#include "TBNR_API.h"
#include "SNI_API.h"
#include "commonFunc.h"

#include <pthread.h>

#include <cassert>
#include <cstdlib>
#include <cstdio>
#include <map>
#include<iostream>
using namespace std;

# ifndef _COMMONFUNC_H
enum BLOG_LEVEL{BLOGT, BLOGD, BLOGI, BLOGW, BLOGE};
#define BLOGLMT BLOGD
#define BIFO(x) if(BLOGLMT <= BLOG##x) 
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

void sni_exit()
{
    SNI_Exit();
}

void sni_init(unsigned thrdnum)
{
    const char * snisysdir = "ioacas/SNISysdir";
    int err = SNI_Init(const_cast<char*>(snisysdir), static_cast<int>(thrdnum));
    if(err !=0){
        BLOGE("failed to call SNI_Init, error: %d.", err);
        exit(1);
    }
    atexit(sni_exit);
}

bool sni_open(int &hdl)
{
    if(SNI_Open(hdl) != 0){
        BLOGE("failed to call SNI_Open");
        return false;
    }
    return true;
}

bool sni_close(int hdl)
{
    if(SNI_Close(hdl) != 0){
        BLOGE("failed to call SNI_Close");
        return false;
    }
    return true;
}

bool isAudioSynthetic(int hdl, short *pcmdata, unsigned pcmlen, int &maxscore)
{
    float scores[3];
    int err = SNI_Recognize(hdl, pcmdata, pcmlen * 2, -1, -1);
    if(err != 0){
        BLOGE("failed to call SNI_Recognize!");
        return false;
    }
    err = SNI_GetResult(hdl, scores, 3);
    if(err != 0){
        BLOGE("failed to call SNI_GetResult.");
    }
    float score = scores[0];
    unsigned maxidx = 0;
    for(unsigned idx=1; idx < 3; idx ++){
        if(score < scores[idx]){
            score = scores[idx];
            maxidx = idx;
        }
    }
    if(maxidx == 1){
        maxscore = score * 100;
        return true;
    }
    else{
        return false;
    }
}

char *str_clone(char *text)
{
    if(text == NULL) return NULL;
    int len = strlen(text);
    char *ret = (char*)malloc(len + 1);
    char *ptr = text;
    char *pch = ret;
    do{
        *(pch++) = *(ptr++);
    }while(*ptr != '\0');
    *pch = '\0';
    return ret;
}

WordResult* WordResult_clone(WordResult *res)
{
    if(res == NULL) return NULL;
    WordResult *word = new WordResult;   
    word->candNum = res->candNum;
    //word->cands = res->cands;

    word->cands = NULL;
    ACand *pcand;
    ACand *pcand1 = res->cands;
    while(pcand1 != NULL){
        if(word->cands == NULL){
            pcand = new ACand;
            word->cands = pcand;
        }
        else{
            pcand->next = new ACand;
            pcand = pcand->next;
        }
        pcand->text = str_clone(pcand1->text);
        pcand->score = pcand1->score;
        pcand->startTime = pcand1->startTime;
        pcand->endTime = pcand1->endTime;
        pcand1 = pcand1->next;
    }
    if(word->cands != NULL) pcand->next = NULL;

    word->next = WordResult_clone(res->next);
    return word;
}
void WordResult_free(WordResult *res)
{
    WordResult *wordptr = res;
    while(wordptr != NULL){
        ACand *candptr = wordptr->cands;
        while(candptr != NULL){
            if(candptr->text != NULL) free(candptr->text);
            ACand *delcand = candptr;
            candptr = candptr->next;
            delete delcand;
        }

        WordResult *delword = wordptr;
        wordptr = wordptr->next;
        delete delword;
    }
}

static const unsigned MaxLineLen = 1024;
string WordResult_toStr(WordResult *res)
{
    char tmpline[MaxLineLen];
    unsigned curlen = 0;
    tmpline[curlen] = '\0';
    curlen += snprintf(tmpline + curlen, MaxLineLen - curlen, res->cands->phone);
    //while(res != NULL){
    //    curlen += snprintf(tmpline + curlen, MaxLineLen - curlen, "[");
    //    ACand *cand = res->cands;
    //    while(cand != NULL){
    //        curlen += snprintf(tmpline + curlen, MaxLineLen - curlen, "%s|", cand->text);
    //        cand = cand->next;
    //    }
    //    if(*(tmpline + curlen) == '|') curlen --;
    //    curlen += snprintf(tmpline + curlen, MaxLineLen - curlen, "]");
    //    res = res->next;
    //}
    return tmpline;
}

typedef WordResult *WordResultPtr;
struct WordResultArr{
    WordResultArr(): res(NULL), len(0)
    {}
    WordResultPtr *res;
    int len;
};

struct TBNRSessionSpace{
    explicit TBNRSessionSpace(int sessid){
        this->sessId = sessid;
        state = IDLE;
        resultSize = 0;
        pthread_mutex_init(&lock, NULL);
        pthread_cond_init(&cond, NULL);
    }
    ~TBNRSessionSpace(){
        pthread_mutex_destroy(&lock);
        pthread_cond_destroy(&cond);
    }

    string serialresult();
    void freeResultes();
    void incrOneResult(const Task *res, int len);
    void dumpresult(const char* kwfile);
    int sessId;
    static const unsigned resultCapacity = 100;
    WordResultArr resultArr[resultCapacity];
    
    int resultSize;
    enum EngineState{IDLE, BUSY, DONE};
    EngineState state;
    pthread_mutex_t lock;
    pthread_cond_t cond;
};

string TBNRSessionSpace::serialresult()
{
    const unsigned maxlen = 1024;
    char tmpline[maxlen];
    int curlen = 0;
    tmpline[0] = '\0';
    for(int idx=0; idx < resultSize; idx++){
        for(int jdx=0; jdx < resultArr[idx].len; jdx++){
            curlen += snprintf(tmpline + curlen, maxlen - curlen, "%s ", WordResult_toStr(resultArr[idx].res[jdx]).c_str());
        }
        curlen += snprintf(tmpline + curlen, maxlen - curlen, ";");
    }
    return tmpline;
}

void TBNRSessionSpace::dumpresult(const char* kwfile)
{
    if(kwfile == NULL || kwfile[0] == '\0'){
        return;
    }
    FILE *fp = fopen(kwfile, "w");
    if(fp == NULL){
        BLOGE("failed to create kwoutfile %s", kwfile);
        return;
    }
    string res = serialresult();
    fwrite(res.c_str(), 1, res.size(), fp);
    fclose(fp);
}

void TBNRSessionSpace::freeResultes()
{
    for(int idx=0; idx < resultSize; idx++){
        for(int jdx=0; jdx < resultArr[idx].len; jdx++){
            WordResult_free(resultArr[idx].res[jdx]);
        }
        delete [] resultArr[idx].res;
    }
    resultSize = 0;
}

void TBNRSessionSpace::incrOneResult(const Task *res, int len)
{
    if(resultSize == resultCapacity){
        BLOGE("in TBNRSessionSpace::incrOneResult, discard result from tbnr, as the result buffer is full.");
        return;
    }
    resultArr[resultSize].res = new WordResultPtr[len];
    resultArr[resultSize].len = len;
    for(int idx=0; idx < len; idx++){
        resultArr[resultSize].res[idx] = WordResult_clone(reinterpret_cast<WordResult*>(res[idx].newSet));
    }
    resultSize ++;
}

static map<int, TBNRSessionSpace*> g_TBNRSessMap;
static pthread_mutex_t g_TBNRSessMapLock = PTHREAD_MUTEX_INITIALIZER;
static string g_TKSysDir = "../../cts_linux_lstm_epd_syllable_1201704114332/model/";
//static string g_TKSysDir = "../model/";
static string g_TKCfgFile = "WFSTDecoder_offline_lstm_onlyrec.cfg";

static inline void insertTBNRSess(TBNRSessionSpace *sp)
{
    pthread_mutex_lock(&g_TBNRSessMapLock);   
    int sessid = sp->sessId;
    assert(g_TBNRSessMap.find(sessid) == g_TBNRSessMap.end());
    g_TBNRSessMap[sessid] = sp;
    pthread_mutex_unlock(&g_TBNRSessMapLock);   
}
static inline TBNRSessionSpace* getTBNRSess(int sessid)
{
    pthread_mutex_lock(&g_TBNRSessMapLock);   
    assert(g_TBNRSessMap.find(sessid) != g_TBNRSessMap.end());
    TBNRSessionSpace *ret = g_TBNRSessMap[sessid];
    pthread_mutex_unlock(&g_TBNRSessMapLock);   
    return ret;
}
static inline void delTBNRSess(int sessid)
{
    pthread_mutex_lock(&g_TBNRSessMapLock);   
    assert(g_TBNRSessMap.erase(sessid) == 1);
    pthread_mutex_unlock(&g_TBNRSessMapLock);   
}

void tbnr_exit()
{
    BLOGD("start tbnr_exit.");
    if(TBNR_Exit() != 0){
        BLOGE("failed to call TBNR_Exit.");
    }
}

bool tbnr_init(int sessnum)
{
    BLOGD("start tbnr_init.");
    int err = TBNR_Init(g_TKSysDir.c_str(), const_cast<char*>(g_TKCfgFile.c_str()), sessnum);
    if(err < 0){
        BLOGE("failed to call TBNR_Init, err: %d", err);
        exit(1);
    }
    atexit(tbnr_exit);
    BLOGI("successful to init tbnr, sessnum: %d", sessnum);
    return true;
}

static void putOneBatchKwRes(const Task *pResultArray, int numberOfTasks, int sessionId)
{
    //for(int idx=0; idx < 1; idx++){
    //    ACand * cand = ((WordResult*)pResultArray[idx].newSet)->cands;
    //    BLOGD("found text in session %d, %s;%s;%s", sessionId, cand->text, cand->phone, cand->segTime);
    //}
    TBNRSessionSpace * sp = getTBNRSess(sessionId);
    sp->incrOneResult(pResultArray, numberOfTasks);
}

static void  putTBNRStatus(int eventID, int sessionId)
{
    if(eventID == TBNR_EVENT_RECOGNITION_COMPLETE){
        TBNRSessionSpace* sp = getTBNRSess(sessionId);
        pthread_mutex_lock(&sp->lock);
        sp->state = TBNRSessionSpace::DONE;
        pthread_mutex_unlock(&sp->lock);
        pthread_cond_signal(&sp->cond);
    }
}

bool tbnr_start(int &sid)
{
    sid = TBNR_Start();
    if(sid < 0){
        BLOGE("failed to call TBNR_Start. ret: %d", sid);
        return false;
    }
    TBNRSessionSpace *sp = new TBNRSessionSpace(sid);
    assert(sp != NULL);
    insertTBNRSess(sp);
    if(TBNR_SetEventCallbackFunc(putTBNRStatus, sid) != 0){
        BLOGE("failed to call TBNR_SetEventCallbackFunc.");
        tbnr_stop(sid);
        return false;
    }
    if(TBNR_SetResultCallbackFunc(putOneBatchKwRes, sid) != 0){
        BLOGE("failed to call TBNR_SetResultCallbackFunc.");
        tbnr_stop(sid);
        return false;
    }
    BLOGI("one session from tbnr_start, id: %d.", sid);
    return true;
}

bool tbnr_stop(int sid)
{
    if(TBNR_Stop(sid) != 0){
        BLOGE("failed to call TBNR_Stop.");
        return false;
    }
    delTBNRSess(sid);
    return true;
}

bool tbnr_recognize(int sid, short *pcmdata, unsigned pcmlen, const char *kwoutfile)
{
    TBNRSessionSpace *sp = getTBNRSess(sid);
    //be sure result field is empty.
    assert(sp->resultSize == 0);
    sp->state = TBNRSessionSpace::BUSY;
    if(TBNR_SendData(reinterpret_cast<char*>(pcmdata), pcmlen *2, sid) < 0){
        BLOGE("failed to call TBNR_SendData.");
        sp->state = TBNRSessionSpace::DONE;
    }
    if(TBNR_StopRecording(sid) < 0){
        BLOGE("failed to call TBNR_StopRecording.");
    }
    pthread_mutex_lock(&sp->lock);
    while(sp->state == TBNRSessionSpace::BUSY){
        pthread_cond_wait(&sp->cond, &sp->lock);
    }
    assert(sp->state == TBNRSessionSpace::DONE);
    pthread_mutex_unlock(&sp->lock);
    //TODO fetch resultes.
    sp->dumpresult(kwoutfile);
    BLOGT("sessionId %d: %s", sid, sp->serialresult().c_str());
    pthread_mutex_lock(&sp->lock);
    sp->state = TBNRSessionSpace::IDLE;
    sp->freeResultes();
    pthread_mutex_unlock(&sp->lock);

    return true;
}
