/*************************************************************************
	> File Name: sni_kw_ex.cpp
	> Author: 
	> Mail: 
	> Created Time: Thu 04 May 2017 10:12:49 PM EDT
 ************************************************************************/

#include "sni_kw_ex.h"
#include "TKW_API.h"
#include "TBNR_API.h"
#include "SNI_API.h"
#include "commonFunc.h"

#include <pthread.h>

#include <cassert>
#include <cstdlib>
#include <cstdio>
#include <set>
#include <map>
#include<iostream>
#include <sstream>
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
    curlen += snprintf(tmpline + curlen, MaxLineLen - curlen, res->cands->text);
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
        resultSize = 0;
	kwResultList.pList = NULL;
	kwResSize = 0;
	kwResCapa = 0;
	indexBufCapa = 40 * 1024;
	indexBuffer = (char*)malloc(indexBufCapa);
	assert(indexBuffer != NULL);
	indexSize = 0;
        //pthread_mutex_init(&lock, NULL);
        //pthread_cond_init(&cond, NULL);
    }
    ~TBNRSessionSpace(){
	free(indexBuffer);
	indexSize = 0;
	indexBufCapa = 0;
        //pthread_mutex_destroy(&lock);
        //pthread_cond_destroy(&cond);
    }

    string serialTBNRResult();
    void freeTBNRResult();
    void incrOneResult(const Task *res, int len);
    string serialTKWResult(string &hitkw);
    void freeTKWResult();
    //void dumpTKWResult(const char *kwfile);

    static const unsigned resultCapacity = 100;
    int sessId;
    WordResultArr resultArr[resultCapacity];
    int resultSize;
    KWRESULTLIST kwResultList;
    unsigned kwResCapa;
    unsigned kwResSize;
    char *indexBuffer;
    unsigned indexSize;
    unsigned indexBufCapa;
    //enum EngineState{IDLE, BUSY, DONE};
    //EngineState state;
    //pthread_mutex_t lock;
    //pthread_cond_t cond;
};

string TBNRSessionSpace::serialTBNRResult()
{
    const unsigned maxlen = 1024;
    char tmpline[maxlen];
    int curlen = 0;
    tmpline[0] = '\0';
    for(int idx=0; idx < resultSize; idx++){
        for(int jdx=0; jdx < resultArr[idx].len; jdx++){
	    WordResult * res = resultArr[idx].res[jdx];
	    if(res == NULL) continue;
            curlen += snprintf(tmpline + curlen, maxlen - curlen, "%s ", WordResult_toStr(res).c_str());
        }
        curlen += snprintf(tmpline + curlen, maxlen - curlen, ";");
    }
    return tmpline;
}

void TBNRSessionSpace::freeTBNRResult()
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
        BLOGE("in TBNRSessionSpace::incrOneResult, discard result from tkw, as the result buffer is full.");
        return;
    }
    resultArr[resultSize].res = new WordResultPtr[len];
    resultArr[resultSize].len = len;
    for(int idx=0; idx < len; idx++){
        resultArr[resultSize].res[idx] = WordResult_clone(reinterpret_cast<WordResult*>(res[idx].latSet));
    }
    resultSize ++;
}

string TBNRSessionSpace::serialTKWResult(string &hitkw)
{
    if(kwResSize == 0) return "";
    ostringstream oss;
    KWRESULT * plist = kwResultList.pList;
    unsigned *idxarr = new unsigned[kwResSize];
    for(unsigned idx=0; idx < kwResSize; idx++){
	idxarr[idx] = idx;
    }
    //insert sort.
    for(unsigned idx=0; idx < kwResSize - 1; idx++){
	unsigned maxidx = idx;
	for(unsigned jdx=idx + 1; jdx < kwResSize; jdx ++){
	    if(plist[idxarr[jdx]].fLogScore > plist[idxarr[maxidx]].fLogScore){
		maxidx= jdx;
	    }
	}
	if(maxidx != idx){
	    unsigned tmpval = idxarr[idx];
	    idxarr[idx] = idxarr[maxidx];
	    idxarr[maxidx] = tmpval;
	}
    }
    hitkw = plist[idxarr[0]].kw;
    for(unsigned idx=0; idx < kwResSize; idx++){
	unsigned i = idxarr[idx];
	//<<"session: "<< plist[i].szSessionID<< "; "
	oss<< "keyword: "<< plist[i].kw<< "; starttime: "<< plist[i].fStartTime<< "; endtime: "<< plist[i].fEndTime<< "; logscore: "<< plist[i].fLogScore<< endl;
    }
    delete []idxarr;
    return oss.str();
}

void TBNRSessionSpace::freeTKWResult()
{
    if(kwResultList.pList != NULL) free(kwResultList.pList);
    kwResultList.pList = 0;
    kwResCapa = 0;
    kwResSize = 0;
}

static map<int, TBNRSessionSpace*> g_TBNRSessMap;
static pthread_mutex_t g_TBNRSessMapLock = PTHREAD_MUTEX_INITIALIZER;
static string g_TKSysDir = "/home/ioacas/cts_linux_lstm_epd_syllable_1201704114332/";
//static string g_TKSysDir = "../model/";
static string g_TKCfgFile = "TKWSys.cfg";
static const char* g_kwfile = "ioacas/zpkw.list";
static KWLIST g_KwList;
static map<string, unsigned> g_mAllKws4Rep;

static void dumpKwEntries(const char *outfile)
{
    FILE *fp = fopen(outfile, "w");
    if(fp == NULL){
	return ;
    }

    int len = g_KwList.nLen;
    KWENTRY *pKwEntry = g_KwList.pList;
    for(int idx=0; idx < len; idx++){
	const char * curkw = pKwEntry[idx].szKeyWordChinese;
	assert(g_mAllKws4Rep.find(curkw) != g_mAllKws4Rep.end());
	fprintf(fp, "%s %u\n", curkw, g_mAllKws4Rep[curkw]);
    }
    fclose(fp);
}

static size_t parsekwsFromLine(char *line, vector<const char*>& arr)
{
    arr.clear();
    char *p = line;
    char *kw = NULL;
    while(*p != '\0'){
        if(strchr(" \t\r\n,;", *p) != NULL){
            if(kw != NULL){
                *p = '\0';
                arr.push_back(kw);
                kw = NULL;
            }
            p++;
            continue;
        }
        if((unsigned char)'\xa1' <= (unsigned char)*p && (unsigned char)*p <= (unsigned char)'\xa9'){
            if(kw != NULL){
                *p = '\0';
                p ++;
                *p = '\0';
                arr.push_back(kw);
                kw = NULL;
            }
            p++;
            continue;
        }
        if(kw == NULL){
            kw = p;
        }
        if((unsigned char)*p > (unsigned char)'\x80'){
            p++;
        }
        p++;
    }
    if(kw != NULL){
        arr.push_back(kw);
    }
    return arr.size();
}


static bool readkwlist(const char *kwfile, map<string, unsigned> &allkws)
{
    FILE *fp = fopen(kwfile, "r");
    if(fp == NULL){
    BLOGE("failed to open keywords list file %s", kwfile);
    return false;
    }
    const unsigned maxLineLen = 1024;
    char tmpline[maxLineLen];
    while(fgets(tmpline, maxLineLen, fp)!= NULL){
        if(tmpline[0] == '\0') continue;
        vector<const char*> arr;
        if(parsekwsFromLine(tmpline, arr) == 0){
            continue;
        }
        unsigned cfgId = (unsigned) -1;
        set<string> curkws;
        for(size_t idx=0; idx < arr.size(); idx++){
            unsigned tmpval;
            if(sscanf(arr[idx], "%u", &tmpval) == 1){
                cfgId = tmpval;
            }
            else{
                curkws.insert(arr[idx]);
            }
        }
        for(set<string>::iterator it=curkws.begin(); it!=curkws.end(); it++){
            allkws[*it] = cfgId;
        }
    }
    fclose(fp);
    if(allkws.size() == 0){
    BLOGE("failed to read any keywords in file %s", kwfile);
    return false;
    }
    BLOGI("in readkwlist, have read keywords, num: %u", allkws.size());
    return true;
}


static void loadKeywords()
{
    if(!readkwlist("ioacas/zpkw.list", g_mAllKws4Rep)){
	exit(1);
    }
    g_KwList.pList = (KWENTRY*)malloc(sizeof(KWENTRY) * g_mAllKws4Rep.size());
    assert(g_KwList.pList != NULL);
    g_KwList.nLen = g_mAllKws4Rep.size();
    KWENTRY *pKwEntry = g_KwList.pList;
    g_KwList.pronmode = (int)(KWLIST_AUTO_GEN_PRON_MODE);
    g_KwList.querymode = KWLIST_STD_QUERY_MODE;
    for(map<string, unsigned>::iterator it= g_mAllKws4Rep.begin(); it != g_mAllKws4Rep.end(); it++){
	strcpy(pKwEntry->szKeyWordChinese, it->first.c_str());
	strcpy(pKwEntry->szKeyWordPinyin, "");
	pKwEntry->pronmode = (int)(KWLIST_AUTO_GEN_PRON_MODE);
	pKwEntry->KWInfor = NULL;
	pKwEntry ++;
    }
    dumpKwEntries("ioacas/zpkw.bck");
}

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


void tkw_exit()
{
    if(TKW_Exit() != 0){
        BLOGE("failed to call TBNR_Exit.");
    }
}

bool tkw_init(int sessnum, const char *kwSysDir)
{
    if(kwSysDir != NULL && *kwSysDir != '\0'){
        g_TKSysDir = kwSysDir;
    }
    BLOGD("start tkw_init.");
    int err = TKW_Init(const_cast<char*>(g_TKSysDir.c_str()), sessnum, const_cast<char*>(g_TKCfgFile.c_str()));
    if(err < 0){
        BLOGE("failed to call TKW_Init, err: %d", err);
        exit(1);
    }
    loadKeywords();
    atexit(tkw_exit);
    BLOGI("successful to init tkw, sessnum: %d", sessnum);
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

static void  putTBNRStatus(int sessionId, EventType eventID, EventInfo *info, void *hook)
{
    BLOGT("Progress CallBack session %s, %.3fs proc", info->SessionID, info->DurationProc);
}
//static void  putTBNRStatus(int eventID, int sessionId)
//{
//    if(eventID == TBNR_EVENT_RECOGNITION_COMPLETE){
//        TBNRSessionSpace* sp = getTBNRSess(sessionId);
//        pthread_mutex_lock(&sp->lock);
//        sp->state = TBNRSessionSpace::DONE;
//        pthread_mutex_unlock(&sp->lock);
//        pthread_cond_signal(&sp->cond);
//    }
//}

bool tkw_open(int &sid)
{
    TKW_ERRCODE err = TKW_Open(&sid, TKW_OFFLINEMODE);
    if(err < 0){
        BLOGE("failed to call TKW_Open. ret: %d", err);
        return false;
    }
    TBNRSessionSpace *sp = new TBNRSessionSpace(sid);
    assert(sp != NULL);
    insertTBNRSess(sp);

    BLOGI("one session from tkw_open, id: %d.", sid);
    return true;
}

bool tkw_close(int sid)
{
    if(TKW_Close(sid) != 0){
        BLOGE("failed to call TKW_Close.");
        return false;
    }
    delTBNRSess(sid);
    return true;
}

bool tkw_recognize(int sid, short *pcmdata, unsigned pcmlen, const char *wavfile, char* &output, unsigned outlen, unsigned &cfgId)
{
    const char *basename = strrchr(wavfile, '/');
    if(basename != NULL) basename ++;
    else basename = wavfile;
    TBNRSessionSpace *sp = getTBNRSess(sid);
    double duration = pcmlen / 8000.0;
    TKW_SetEventCallback(sid, putTBNRStatus, &duration);

    SESSIONINFOR sessinfo;
    strcpy(sessinfo.szSessionID, basename);
    sessinfo.fTimeStamp = 0;
    strcpy(sessinfo.szSessionDir, "");
    strcpy(sessinfo.szSerialNumber, "");
    TKW_ERRCODE err = TKW_Start(sid, &sessinfo);
    if(err < 0){
	BLOGE("in tkw_recognize, failed to call TKW_Start, sid: %d err: %d", sid, err);
	return false;
    }
    if(TBNR_SetRecResultCallbackFunc(putOneBatchKwRes, sid) != 0){
        BLOGE("failed to call TBNR_SetResultCallbackFunc.");
	TKW_Stop(sid, 0);
        return false;
    }
    SPEECHBUF speechbuf;
    speechbuf.buf = reinterpret_cast<char*>(pcmdata);
    speechbuf.nByteLen = pcmlen;
    speechbuf.type = TKW_8K_16BIT_PCM;
    err = TKW_Recognize(sid, &speechbuf);
    if(err < 0){
	BLOGE("in tkw_recognize, failed to call TKW_Recognize, err:%d", err);
	TKW_Stop(sid, 0);
	return false;
    }
    char *indexBuf = NULL;
    int indexSize = 0;
    err = TKW_Stop_Mem(sid, indexBuf, indexSize);
    if(err < 0){
	BLOGE("in tkw_recognize, failed to call TKW_Stop, err: %d", err);
	return false;
    }
    if(indexSize > sp->indexBufCapa){
    sp->indexBufCapa += 2 * indexSize;
    sp->indexBuffer = (char*)realloc(sp->indexBuffer, sp->indexBufCapa);
    }
    memcpy(sp->indexBuffer, indexBuf, indexSize);
    sp->indexSize = indexSize;
    TKW_ReleaseIdxMem(sid);

    string tbnrres = sp->serialTBNRResult();
    BLOGT("TBNR sessionId %d: %s %s", sid, wavfile, tbnrres.c_str());
    sp->freeTBNRResult();
    if(g_KwList.pList == NULL){
	BLOGI("in tkw_recognize, skip keyword search.");
	return true;
    }
    err = TKW_SetKeyWordList(sid, &g_KwList);
    if(err < 0){
	BLOGE("in tkw_recognize, failed to call TKW_SetKeyWordList, err: %d", err);
	return false;
    }

    SEARCHINFOR_M searchinfo;
    sprintf(searchinfo.szSessionID, "%s", basename);
    searchinfo.pIndexBuf = sp->indexBuffer;
    searchinfo.szIndexBufLen = sp->indexSize;
    searchinfo.startTimeStamp = 0;
    searchinfo.endTimeStamp = -1;

    int counter = 0;
    err = TKW_Search_Mem(sid, &searchinfo, &counter, TKW_SEARCH_CUR_SESSION);
    if(err < 0){
	BLOGE("in tkw_recognize, failed to call TKW_Search, err: %d", err);
	return false;
    }
    BLOGI("after TKW_Search, kwnum: %d, counter: %d.", g_KwList.nLen, counter);
    if(counter > 0){
	KWRESULTLIST &kwres = sp->kwResultList;
	if(counter > sp->kwResCapa){
	    kwres.pList = (KWRESULT*)realloc(kwres.pList, (counter + 10) * sizeof(KWRESULT));
	    sp->kwResCapa = counter;
	    assert(kwres.pList != NULL);
	}
	sp->kwResSize = counter;
	err = TKW_GetKwResult(sid, &kwres, &counter, TKW_RESULT_SORT_SESSIONNAME_STARTTIME);
	string hitkw;
	string kwresbuf = sp->serialTKWResult(hitkw);
	string resfirstline;
	size_t delipos = kwresbuf.find("\n");
	if(delipos == string::npos){
	    resfirstline = kwresbuf;
	}
	else{
	    resfirstline = kwresbuf.substr(0, delipos);
	}
	BLOGT("TKW sessionId %d: %s %s", sid, wavfile, resfirstline.c_str());
	assert(g_mAllKws4Rep.find(hitkw) != g_mAllKws4Rep.end());
    if(g_mAllKws4Rep[hitkw] != (unsigned)-1) cfgId = g_mAllKws4Rep[hitkw];
	snprintf(output, outlen, "%s\n%s\n", tbnrres.c_str(), kwresbuf.c_str()); 
    }
    return true;
}

/*
static void dumpkwres(const char *kwfile, string &res)
{
    if(kwfile == NULL || kwfile[0] == '\0'){
        return;
    }
    FILE *fp = fopen(kwfile, "a");
    if(fp == NULL){
        BLOGE("failed to create kwoutfile %s", kwfile);
        return;
    }
    fwrite(res.c_str(), 1, res.size(), fp);
    fclose(fp);
}

bool tkw_recognize(int sid, short *pcmdata, unsigned pcmlen, const char *wavfile)
{
    const char *basename = strrchr(wavfile, '/');
    if(basename != NULL) basename ++;
    else basename = wavfile;
    const char *lastsep = strrchr(wavfile, '.');
    string kwfile = string(wavfile, lastsep);
    kwfile += ".txt";

    TBNRSessionSpace *sp = getTBNRSess(sid);
    SESSIONINFOR sessinfo;
    strcpy(sessinfo.szSessionID, basename);
    sessinfo.fTimeStamp = 0;
    strcpy(sessinfo.szSessionDir, "");
    strcpy(sessinfo.szSerialNumber, "");
    TKW_ERRCODE err = TKW_Start(sid, &sessinfo);
    if(err < 0){
	BLOGE("in tkw_recognize, failed to call TKW_Start, err: %d", err);
	return false;
    }
    if(TBNR_SetRecResultCallbackFunc(putOneBatchKwRes, sid) != 0){
        BLOGE("failed to call TBNR_SetResultCallbackFunc.");
	TKW_Stop(sid, 0);
        return false;
    }
    SPEECHBUF speechbuf;
    speechbuf.buf = reinterpret_cast<char*>(pcmdata);
    speechbuf.nByteLen = pcmlen;
    speechbuf.type = TKW_8K_16BIT_PCM;
    err = TKW_Recognize(sid, &speechbuf);
    if(err < 0){
	BLOGE("in tkw_recognize, failed to call TKW_Recognize, err:%d", err);
	TKW_Stop(sid, 0);
	return false;
    }
    err = TKW_Stop(sid, 0);
    if(err < 0){
	BLOGE("in tkw_recognize, failed to call TKW_Stop, err: %d", err);
	return false;
    }
    string tbnrres = sp->serialTBNRResult();
    BLOGI("TBNR sessionId %d: %s %s", sid, wavfile, tbnrres.c_str());
    sp->freeTBNRResult();
    if(g_KwList.pList == NULL){
	BLOGI("in tkw_recognize, skip keyword search.");
	return true;
    }
    err = TKW_SetKeyWordList(sid, &g_KwList);
    if(err < 0){
	BLOGE("in tkw_recognize, failed to call TKW_SetKeyWordList, err: %d", err);
	return false;
    }
    SEARCHINFOR searchinfo;
    sprintf(searchinfo.szSessionID, "%s", basename);
    strcpy(searchinfo.szSessionDir, "");
    strcpy(searchinfo.szSessionWavPath, wavfile);
    searchinfo.startTimeStamp = 0;
    searchinfo.endTimeStamp = -1;
    searchinfo.type = TKW_8K_16BIT_PCM;

    int counter = 0;
    err = TKW_Search(sid, &searchinfo, &counter, TKW_SEARCH_CUR_SESSION);
    if(err < 0){
	BLOGE("in tkw_recognize, failed to call TKW_Search, err: %d", err);
	return false;
    }
    BLOGI("after TKW_Search, kwnum: %d, counter: %d.", g_KwList.nLen, counter);
    if(counter > 0){
	KWRESULTLIST &kwres = sp->kwResultList;
	if(counter > sp->kwResCapa){
	    kwres.pList = (KWRESULT*)realloc(kwres.pList, (counter + 10) * sizeof(KWRESULT));
	    sp->kwResCapa = counter;
	    assert(kwres.pList != NULL);
	}
	sp->kwResSize = counter;
	err = TKW_GetKwResult(sid, &kwres, &counter, TKW_RESULT_SORT_SESSIONNAME_STARTTIME);
	string kwresbuf = sp->serialTKWResult();
	string resfirstline;
	size_t delipos = kwresbuf.find("\n");
	if(delipos == string::npos){
	    resfirstline = kwresbuf;
	}
	else{
	    resfirstline = kwresbuf.substr(0, delipos);
	}
	BLOGI("TKW sessionId %d: %s %s", sid, wavfile, resfirstline.c_str());
	dumpkwres(kwfile.c_str(), tbnrres);
	dumpkwres(kwfile.c_str(), kwresbuf);
    }
    return true;
}
*/
