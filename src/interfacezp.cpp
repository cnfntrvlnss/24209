/*************************************************************************
    > File Name: interface242.cpp
    > Author: zhengshurui
    > Mail:  zhengshurui@thinkit.cn
    > Created Time: Tue 06 Sep 2016 03:31:17 AM PDT
 ************************************************************************/

#include "sni_kw_ex.h"
#include "libBAI_ex.h"
#include "dllSRVADCluster_lshj.h"
#include "commonFunc.h"
#include "wav/waveinfo.h"
#include "utilites.h"
#include "ProjectBuffer.h"
#include "../../serv242_09/src/audiz/audizcli_p.h"

#include <set>

using namespace std;
using namespace zen4audio;

static const unsigned BLOCKSIZE = 3 * 16000;
//TODO 这两个标记变量需要加锁.
static bool g_bInitialized = false;
static unsigned int g_iModuleID;

static ReceiveResult g_ReportResultAddr;
const unsigned int g_uVADType = 0x97;
const unsigned int g_uVADID = 0x01;
const unsigned int g_uMusicType = 0x97;
const unsigned int g_uMusicID = 0x02;
const unsigned int g_uLangWeirType = 0x97;
const unsigned int g_uLangWeirID = 0x20; 
const unsigned int g_uLangTurkType = 0x9a;
const unsigned int g_uLangTurkID = 0x30;
const unsigned int g_uLangAlabType = 0x9b;
const unsigned int g_uLangAlabID = 0x40;
const unsigned short g_uSpkServType = 0x92;
unsigned dft_getTypeFromId(unsigned int id)
{
    if(id == g_uLangWeirID) return g_uLangWeirType;
    else if(id == g_uLangTurkID) return g_uLangTurkType;
    else if(id == g_uLangAlabID) return g_uLangAlabType;
    else if(id == g_uVADID) return g_uVADType;
    else if(id == g_uMusicID) return g_uMusicType;
    else return g_uLangWeirType;
}

std::vector<std::pair<unsigned int, std::pair<unsigned int, unsigned int> > > g_mLangReports;
static std::map<std::pair<unsigned int, unsigned int>, int> g_mLangReportFilter;

bool g_bUseBamp = true;
static unsigned g_uBampVadNum = 0;
static float g_fAfterBampVadSecs = 0.05;
const char szIoacasDir[]="./ioacas/";
char m_TSI_SaveTopDir[MAX_PATH]= "/home/ioacas/back_wave/";
ConfigRoom g_AutoCfg(((string)szIoacasDir + "SysFunc.cfg").c_str());

static bool g_bDiscardable=true;// when feeding data.
bool g_bSaveAfterRec=false; // when after processing, for project ID.

LoggerId g_logger;
string g_strIp;

//std::map<unsigned long,ProjRecord_t> NewReportedID;
//pthread_mutex_t g_lockNewReported = PTHREAD_MUTEX_INITIALIZER;

//////////////////---bamp---
//NOTE: only support 3 secs segment.
static unsigned g_uBampFixedLen = 3 * PCM_ONESEC_LEN;

////////////>>> utilities
static bool saveWave(vector<DataBlock>& prjData, const char *savedfile)
{
    FILE *fp = fopen(savedfile, "ab");
    if(fp == NULL){
        LOG_WARN(g_logger, "fail to open file "<< savedfile<< " error: "<< strerror(errno));
        return false;
    }
    bool ret = false;
    PCM_HEADER pcmheader;
    initialize_wave_header(&pcmheader, 0);
    int retw = fwrite(&pcmheader, sizeof(PCM_HEADER), 1, fp);
    if(retw != 1){
        LOG_WARN(g_logger, "fail to save data to file, filename: "<< savedfile);
        fclose(fp);
        return ret;
    }
    unsigned len = 0;
    for(unsigned idx=0; idx < prjData.size(); idx++){
        len + prjData[idx].len;
        fwrite(prjData[idx].getPtr(), 1, prjData[idx].len, fp);
        
    }
    ret = true;
    fclose(fp);
    return ret;
}

///////////<<<
/**
 * TODO 两个参数的含义还是不够明确，不知道怎么写这个函数.
 */
int GetDLLVersion(char *p, int &length)
{
    static char g_strVersion[100];
    strcpy(g_strVersion, "ioacas/lang v0.1.0");
    length = strlen(g_strVersion);
    strncpy(p, g_strVersion, length);
    return 1;
}

/*
void* IoaRegThread(void *pvParam);

static std::vector<std::pair<unsigned char, unsigned char> > parseLangReportsFromStr(const char*strLine)
{
	char tmpLine[256];
	std::vector<std::pair<unsigned char, unsigned char> > retm;
	strncpy(tmpLine, strLine, 256);
	char *st = tmpLine;
	while(true){
		char *tkEnd = strchr(st, ',');
		if(tkEnd != NULL) {
			*tkEnd = '\0';
		}
		unsigned char f,s;
		if(sscanf(st, "%hhu->%hhx", &f, &s) == 2){
			retm.push_back(std::make_pair(f,s));
		}
		if(tkEnd == NULL) break;
		st = tkEnd + 1;
	}
	return retm;
}

static std::string formLangReportsStr(const std::vector<std::pair<unsigned char, unsigned char> >& langReports)
{
	ostringstream oss;
	for(unsigned i=0; i< langReports.size(); i++){
		oss<< (int)langReports[i].first<< "->" << std::hex<< std::showbase<< (int)langReports[i].second<< std::dec<< std::noshowbase << ",";
	}
	return oss.str();
}

static bool parseReportFilter(const char *strLine, std::map<int, int> &filter)
{
    const char *st = strLine;
    int num1, num2;
    unsigned char tmpCh;
    while(true){
        if(sscanf(st, "%hhx:%d", &tmpCh, &num2) == 2){
            num1 = tmpCh;
            filter[num1] = num2;
        }
        st = strchr(st, ',');
        if(st == NULL) break;
        st ++;
    }
    return true;
}
static std::string formReportFilterStr(const std::map<int, int> &filter)
{
    std::ostringstream oss;
    for(std::map<int,int>::const_iterator it=filter.begin(); it != filter.end(); it++){
        oss<< std::hex<< std::showbase<< it->first<< std::dec<< std::noshowbase<< ":"<< it->second <<",";
    }
    return oss.str();
}
*/

struct SpkMdlStVecImpl: public SpkMdlStVec, SpkMdlStVec::iterator
{
    SpkMdlStVecImpl* iter(){
        return this;
    }
    SpkMdlSt *next(){
        return NULL;
    }
};
static SpkMdlStVecImpl g_DummySpkVec;
static char g_AudizPath[MAX_PATH] = "../ioacases/dataCenter";
static bool g_bUseRecSess = false;
static SessionStruct *g_RecSess = NULL;

static void initGlobal(BufferConfig &myBufCfg)
{
    g_strIp = GetLocalIP();
    Config_getValue(&g_AutoCfg, "", "ifSkipSameProject", g_bSaveAfterRec);
    Config_getValue(&g_AutoCfg, "", "savePCMTopDir", m_TSI_SaveTopDir);
    Config_getValue(&g_AutoCfg, "", "ifUseRecSess", g_bUseRecSess);
    Config_getValue(&g_AutoCfg, "", "audizCenterDataPath", g_AudizPath);
    Config_getValue(&g_AutoCfg, "bamp", "ifUseBAMP", g_bUseBamp);
    Config_getValue(&g_AutoCfg, "bamp", "reportBampThreshold", g_fReportBampThrd);
    Config_getValue(&g_AutoCfg, "bamp", "bampVadThreadNum", g_uBampVadNum);
    Config_getValue(&g_AutoCfg, "bamp", "afterBampVadSeconds", g_fAfterBampVadSecs);
    Config_getValue(&g_AutoCfg, "bamp", "baiThreadNum", g_uBampThreadNum);
    Config_getValue(&g_AutoCfg, "bamp", "serverBampIp", g_szBampIp);
    Config_getValue(&g_AutoCfg, "bamp", "serverBampPort", g_uBampPort);

    Config_getValue(&g_AutoCfg, "projectBuffer", "ifDiscardable", g_bDiscardable);
    Config_getValue(&g_AutoCfg, "projectBuffer", "waitSecondsStep", myBufCfg.waitSecondsStep);
    Config_getValue(&g_AutoCfg, "projectBuffer", "waitSeconds", myBufCfg.waitSeconds);
    Config_getValue(&g_AutoCfg, "projectBuffer", "waitLength", myBufCfg.waitLength);
    //Config_getValue(&g_AutoCfg, "projectBuffer", "bufferBlockSize", myBufCfg.m_uBlockSize);
    Config_getValue(&g_AutoCfg, "projectBuffer", "bufferBlocksMin", myBufCfg.m_uBlocksMin);
    Config_getValue(&g_AutoCfg, "projectBuffer", "bufferBlocksMax", myBufCfg.m_uBlocksMax);
    if(g_szBampIp[0] == '\0') strncpy(g_szBampIp, g_strIp.c_str(), 50);
    myBufCfg.waitLength *= 16000;
	
	unsigned tmpLen = strlen(m_TSI_SaveTopDir);
	if(m_TSI_SaveTopDir[tmpLen - 1] != '/'){
		m_TSI_SaveTopDir[tmpLen] = '/';
		m_TSI_SaveTopDir[tmpLen + 1] = '\0';
	}
	g_logger = g_Log4zManager->createLogger("ioacas");
    char strVer[50];
    int verLen = 50;
    GetDLLVersion(strVer, verLen);
    LOG_INFO(g_logger, "version --- "<< strVer);
#define LOG4Z_VAR(x) << #x "=" << x << "\n"
    LOG_INFO(g_logger, "====================config====================\n" 
            LOG4Z_VAR(g_AutoCfg.configFile)
            LOG4Z_VAR(m_TSI_SaveTopDir)
            LOG4Z_VAR(g_bUseBamp)
            LOG4Z_VAR(g_fReportBampThrd)
            LOG4Z_VAR(g_uBampThreadNum)
            LOG4Z_VAR(g_uBampVadNum)
            LOG4Z_VAR(g_fAfterBampVadSecs)
            LOG4Z_VAR(g_szBampIp)
            LOG4Z_VAR(g_uBampPort)
            LOG4Z_VAR(g_bDiscardable)
            LOG4Z_VAR(g_bSaveAfterRec)
            LOG4Z_VAR(g_AutoCfg.configFile)
            LOG4Z_VAR(myBufCfg.waitSecondsStep)
            LOG4Z_VAR(myBufCfg.waitSeconds)
            LOG4Z_VAR(myBufCfg.waitLength )
            LOG4Z_VAR(myBufCfg.m_uBlocksMin)
            LOG4Z_VAR(myBufCfg.m_uBlocksMax)
            );
}

struct KwMatchSpace{
    explicit KwMatchSpace(ProjectBuffer *p):
        prj(p), curStIdx(0), dataLen(0)
    {
        prjTime = prj->getPrjTime();
        bufSize = 16000 * 6;
        afterVADBuf = (char*)malloc(bufSize + BLOCKSIZE);
        assert(afterVADBuf != NULL);
        //pthread_mutex_init(&anoKwLock, NULL);
    }
    ~KwMatchSpace(){
        //pthread_mutex_destroy(&anoKwLock);
        free(afterVADBuf);
        returnBuffer(prj);
    }
    
    void saveAudio(unsigned short cfgType, unsigned cfgId, int score, char* output, int leftsize);
    ProjectBuffer *prj;
    struct timeval prjTime;
    vector<DataBlock> prjData;
    unsigned curStIdx;
    char *afterVADBuf;
    unsigned bufSize;
    unsigned dataLen;
    string kwoutfile;
    //pthread_mutex_t anoKwLock;
};

void KwMatchSpace::saveAudio(unsigned short cfgType, unsigned cfgId, int score, char* output, int leftsize)
{
    char vadedfile[MAX_PATH];
    gen_spk_save_file(vadedfile, m_TSI_SaveTopDir, NULL, prjTime.tv_sec, prj->ID, &cfgType, &cfgId, &score);
    char *lastdotptr = strrchr(vadedfile, '.');
    sprintf(lastdotptr, "%s", ".raw");
    FILE *fp = fopen(vadedfile, "wb");
    if(fp != NULL){
        fwrite(afterVADBuf, 1, dataLen, fp);
        fclose(fp);
        kwoutfile = string(vadedfile, lastdotptr);
        kwoutfile +=  ".txt";
        int plen = snprintf(output, leftsize, "VADFile=%s ", vadedfile);
        output += plen;
        leftsize -= plen;
    }
    else{
        LOG_ERROR(g_logger, "failed to create vadedfile "<< vadedfile);
    }
    char savedfile[MAX_PATH];
    gen_spk_save_file(savedfile, m_TSI_SaveTopDir, NULL, prjTime.tv_sec, prj->ID, &cfgType);
    if(saveWave(prjData, savedfile)){
        snprintf(output, leftsize, "SAVEDFile=%s ", savedfile);
    }
}

struct KwMatchThreadSpace{
    KwMatchThreadSpace(){
        idx = 0;
        nextProjID = 0;
        pthread_mutex_init(&splock, NULL);
    }
    ~KwMatchThreadSpace(){
        pthread_mutex_destroy(&splock);
    }

    bool addKwSpace(ProjectBuffer *prj){
        pthread_mutex_lock(&splock);
        assert(batchOfProjs.find(prj->ID) == batchOfProjs.end());
        batchOfProjs[prj->ID] = new KwMatchSpace(prj);
        pthread_mutex_unlock(&splock);
        return true;
    }
    /**
     * 返回下一个待处理的节目；若到达了末尾，返回NULL.
     */
    KwMatchSpace *getNextKwSpace(){
        KwMatchSpace *ret = NULL;
        pthread_mutex_lock(&splock);
        map<unsigned long, KwMatchSpace*>::iterator it = batchOfProjs.lower_bound(nextProjID);
        if(it == batchOfProjs.end()){
            nextProjID = 0;
        }
        else{
            ret = it->second;
            nextProjID = ret->prj->ID;
        }
        pthread_mutex_unlock(&splock);
        return ret;
    }
    void delKwSpace(KwMatchSpace *kwsp){
        pthread_mutex_lock(&splock);
        assert(batchOfProjs.erase(kwsp->prj->ID) == 1);
        pthread_mutex_unlock(&splock);
        delete kwsp;
    }
    unsigned idx;   
    map<unsigned long, KwMatchSpace*> batchOfProjs;
    pthread_mutex_t splock;
    unsigned long nextProjID;
private:
    KwMatchThreadSpace(const KwMatchThreadSpace&);
    KwMatchThreadSpace& operator=(const KwMatchThreadSpace&);
};

static KwMatchThreadSpace* g_AllProjs4Kw;
static unsigned g_AllProjs4KwSize = 4;

static void * bampMatchThread(void *);
static void * KwMatchThread(void *);
static void *dummyRecThread(void *);
static void *ioacas_maintain_procedure(void *);
static void reportBampResultSeg(BampResultParam param, ostream& oss);
static int addBampProj(ProjectBuffer *proj);

int InitDLL(int iPriority,
        int iThreadNum,
        int *pThreadCPUID,
        ReceiveResult func,
        int iDataUnitBufSize,
        char *path,
        unsigned int iModuleID)
{
    if(g_bInitialized) {
        LOGE("ioacas module is already initialized.");
        return 0;
    }
    g_iModuleID = iModuleID;
    g_ReportResultAddr = func;
    
    BufferConfig buffconfig;
    buffconfig.m_uBlockSize = BLOCKSIZE;
    buffconfig.m_uBlocksMin = 20 * 600;
    buffconfig.m_uBlocksMax = 20 * 600;
    initGlobal(buffconfig);

    init_bufferglobal(buffconfig, addBampProj);
    {
        pthread_attr_t threadAttr;
        pthread_attr_init(&threadAttr);
        pthread_attr_setdetachstate(&threadAttr, PTHREAD_CREATE_DETACHED);
        pthread_t bmThdId;
        int retc = pthread_create(&bmThdId, &threadAttr, dummyRecThread, NULL);
        if(retc != 0){
            LOG_ERROR(g_logger, "fail to create dummyRecThread thread!");
            exit(1);
        }
        pthread_attr_destroy(&threadAttr);
    }

    if(!InitVADCluster_File()){
        LOG_ERROR(g_logger, "failed to initVADCluster_File.");
    }

    if(g_AllProjs4KwSize > 0){
        g_AllProjs4Kw = new KwMatchThreadSpace[g_AllProjs4KwSize];
        sni_init(g_AllProjs4KwSize);
        tbnr_init(g_AllProjs4KwSize);
        for(unsigned idx=0; idx < g_AllProjs4KwSize; idx++)
        {
            pthread_attr_t threadAttr;
            pthread_attr_init(&threadAttr);
            pthread_attr_setdetachstate(&threadAttr, PTHREAD_CREATE_DETACHED);
            pthread_t bmThdId;
            g_AllProjs4Kw[idx].idx = idx;
            int retc = pthread_create(&bmThdId, &threadAttr, KwMatchThread, &g_AllProjs4Kw[idx]);
            if(retc != 0){
                LOG_ERROR(g_logger, "fail to create KwMatchThread thread!");
                exit(1);
            }
            pthread_attr_destroy(&threadAttr);
        }
    }
    if(g_bUseBamp){
        if(!bamp_init(reportBampResultSeg, g_uBampVadNum, g_fAfterBampVadSecs)){
            LOG_INFO(g_logger, "fail to initailize bamp engine.");
            return 1;
        }
        {
            pthread_attr_t threadAttr;
            pthread_attr_init(&threadAttr);
            pthread_attr_setdetachstate(&threadAttr, PTHREAD_CREATE_DETACHED);
            pthread_t bmThdId;
            int retc = pthread_create(&bmThdId, &threadAttr, bampMatchThread, NULL);
            if(retc != 0){
                LOG_ERROR(g_logger, "fail to create BampMatch thread!");
                exit(1);
            }
            pthread_attr_destroy(&threadAttr);
        }
    }

    if(true)
    {
		pthread_attr_t threadAttr;
		pthread_attr_init(&threadAttr);
        pthread_attr_setdetachstate(&threadAttr, PTHREAD_CREATE_DETACHED);
        pthread_t mthrdId;
        int retc = pthread_create(&mthrdId, &threadAttr, ioacas_maintain_procedure, NULL);
        if(retc != 0){
            LOG_ERROR(g_logger, "fail to create maintain thread!");
            exit(1);
        }
		pthread_attr_destroy(&threadAttr);
    }

    if(g_bUseRecSess){
        g_RecSess = new SessionStruct(g_AudizPath, NULL, &g_DummySpkVec);
    }
    g_bInitialized = true;
    return 0;
}

int SendData2DLL(WavDataUnit *p)
{
    const unsigned HEADSIZE = 100;
    char szHead[HEADSIZE];
    snprintf(szHead, HEADSIZE, "SendData2DLL<%lu> PID=%lu SIZE=%u ", pthread_self(), p->m_iPCBID, p->m_iDataLen);
    if(!g_bInitialized){
        LOG_WARN(g_logger, szHead<<"fail to receive data as ioacas being uninitialized.");
        return -1;
    }
    assert(p->m_iDataLen == g_uBampFixedLen);
    string outputr;
    clockoutput_start("SendData2DLL");
    ProjectSegment prj;
    prj.pid = p->m_iPCBID;
    prj.data = p->m_pData;
    prj.len = p->m_iDataLen;
    recvProjSegment(prj, !g_bDiscardable);
    if(g_RecSess != NULL){
        clockoutput_start("SendData2Remote");
        Audiz_WaveUnit unit;
        unit.m_iPCBID = prj.pid;
        unit.m_iDataLen = prj.len;
        unit.m_pData = prj.data;
        g_RecSess->writeData(&unit);
        outputr = clockoutput_end();
    }
    string output = clockoutput_end();
    LOGFMTT("%s %s", output.c_str(), outputr.c_str());

    return 0;
}

static bool saveWaveAsAlaw(FILE *fp, vector<DataBlock> &blks);
static void appendDataToReportFile(BampMatchParam &par)
{
    if(par.bPreHit){
        char savedfile[MAX_PATH];
        gen_spk_save_file(savedfile, m_TSI_SaveTopDir, NULL, par.curtime.tv_sec, par.pid, NULL, NULL, NULL);
        char *stSufPtr = strrchr(savedfile, '.');
        strcpy(stSufPtr+1, "pcm");
        FILE *fp = fopen(savedfile, "a");
        if(fp == NULL){
            LOGFMT_ERROR(g_logger, "BampMatchThread failed to open file to append data. file: %s\n", savedfile);
        }
        else {
             saveWaveAsAlaw(fp, par.data);   
	     fclose(fp);
        }
    }
    else if(par.bHit){
        vector<DataBlock> prjData;
        par.ptrBuf->getDataSegment(prjData, 0, 0, par.preIdx - 1, par.preOffset);
        char savedfile[MAX_PATH];
        gen_spk_save_file(savedfile, m_TSI_SaveTopDir, NULL, par.curtime.tv_sec, par.pid, NULL, NULL, NULL);
        char *stSufPtr = strrchr(savedfile, '.');
        strcpy(stSufPtr+1, "pcm");
        FILE *fp = fopen(savedfile, "a");
        if(fp == NULL){
            LOGFMT_ERROR(g_logger, "BampMatchThread failed to open file to append data. file: %s\n", savedfile);
        }
        else {
            if(prjData.size() > 0) saveWaveAsAlaw(fp, prjData);
             saveWaveAsAlaw(fp, par.data);   
	     fclose(fp);
        }
    }
}

static bool VADBuffer_fork(const bool bAllOut, const short* psPCMBuffer, const int iSampleNum, short* psPCMBufferVAD, int& riSampleNumVAD, bool bUseDetector=true)
{
    LOGFMT_INFO(g_logger, "in vadbuffer_fork!!!");
    memcpy(psPCMBufferVAD, psPCMBuffer, iSampleNum * 2);
    riSampleNumVAD = iSampleNum;
    return true;
}

void *KwMatchThread(void *param)
{
    int hdl;
    sni_open(hdl);
    int tbnrsid;
    tbnr_start(tbnrsid);
    KwMatchThreadSpace *thrdSp = reinterpret_cast<KwMatchThreadSpace*>(param);
    while(true){
        KwMatchSpace *curSp = thrdSp->getNextKwSpace();
        if(curSp == NULL){
            sleep(1);
            continue;
        }
        vector<DataBlock> &prjdata = curSp->prjData;
        unsigned &curidx = curSp->curStIdx;
        char *&vadbuf = curSp->afterVADBuf;
        unsigned &vadlen = curSp->dataLen;
        const unsigned &vadbufsize = curSp->bufSize;

        curSp->prj->getDataSegment(prjdata, prjdata.size(), 0);
        while(curidx < prjdata.size()){
            short *inSmp = reinterpret_cast<short*>(prjdata[curidx].getPtr());
            unsigned inlen = prjdata[curidx].len / 2;
            short *bufSt = reinterpret_cast<short*>(vadbuf + vadlen);
            unsigned leftsize = (vadbufsize - vadlen) / 2;
            int vadstep = leftsize;
            if(VADBuffer(true, inSmp, inlen, bufSt, vadstep)){
                assert(vadstep >= 0);
                vadlen += vadstep * 2;
                if(vadlen >= vadbufsize){
                    break;
                }
            }
            curidx++;
        }
        
        if(vadlen >= vadbufsize || curSp->prj->getFull()){
            if(vadlen < vadbufsize){
                //TODO under 6s, be discarded before recognition. 
                LOGFMT_INFO(g_logger, "SNIREG PID=%lu WaveLen=%u VADLen=%.2f too short to start SSRec.", curSp->prj->ID, ((prjdata.size()) * BLOCKSIZE) / 16000, (float)vadlen / 16000);
            }
            else{
                //pass through systhesized speech recognition.
                char tmpline[1024];
                int linelen = 0;
                linelen += snprintf(tmpline + linelen, 1024 - linelen, "SNIREC PID=%lu WaveLen=%u VADLen=%.2f ", curSp->prj->ID, ((curidx+1) * BLOCKSIZE) / 16000, (float)vadlen / 16000);
                LOGFMT_DEBUG(g_logger, "%sstart SSRec!", tmpline);
                int synScore;
                if(isAudioSynthetic(hdl, reinterpret_cast<short*>(vadbuf), vadlen / 2, synScore)){
                    linelen += snprintf(tmpline + linelen, 1024 - linelen, "CFGID=%u SCORE=%d ", 1, synScore);
                    curSp->saveAudio(5, 1, synScore, tmpline + linelen, 1024 - linelen);
                    tbnr_recognize(tbnrsid, reinterpret_cast<short*>(vadbuf), vadlen / 2, curSp->kwoutfile.c_str());
                    LOGFMT_INFO(g_logger, "%sEOP", tmpline);
                }
                else{
                }
            }
            thrdSp->delKwSpace(curSp);
        }
    }
    tbnr_stop(tbnrsid);
    sni_close(hdl);
}

static inline unsigned hashPid(unsigned long pid)
{
    unsigned count = 0;
    while(pid > 0){
        pid = pid & (pid - 1);
        count ++;
    }
    return count % g_AllProjs4KwSize;
}

static map<unsigned long, ProjectBuffer*> g_AllProjs4Bamp;
static pthread_mutex_t g_AllProjs4BampLocker = PTHREAD_MUTEX_INITIALIZER;
static int addBampProj(ProjectBuffer *proj)
{
    int ret = 0;

    if(g_bUseBamp){
        pthread_mutex_lock(&g_AllProjs4BampLocker);
        assert(g_AllProjs4Bamp.find(proj->ID) == g_AllProjs4Bamp.end());
        g_AllProjs4Bamp[proj->ID] = proj;
        pthread_mutex_unlock(&g_AllProjs4BampLocker);
        ret++;
    }

    if(g_AllProjs4KwSize > 0){
        unsigned which = hashPid(proj->ID);
        if(g_AllProjs4Kw[which].addKwSpace(proj)){
            ret ++;
        }
    } 

    return ret;
}

static ProjectBuffer *getNextBampProj()
{
    static unsigned long nextProjID;
    ProjectBuffer *ret = NULL;
    pthread_mutex_lock(&g_AllProjs4BampLocker);
    map<unsigned long, ProjectBuffer*>::iterator it = g_AllProjs4Bamp.lower_bound(nextProjID);
    if(it == g_AllProjs4Bamp.end()){
        nextProjID = 0;
    }
    else{
        ret = it->second;
        nextProjID = ret->ID + 1;
    }
    pthread_mutex_unlock(&g_AllProjs4BampLocker);
    return ret;
}

static void delBampProj(ProjectBuffer *prj)
{
    pthread_mutex_lock(&g_AllProjs4BampLocker);
    assert(g_AllProjs4Bamp.erase(prj->ID) == 1);
    pthread_mutex_unlock(&g_AllProjs4BampLocker);
    returnBuffer(prj);
}

/***
 * keep doing bampMatch until projectBuffer is full.
 *
 */
void * bampMatchThread(void *)
{
    BampMatchObject *obj = openBampHandle();
    if(obj == NULL){
        LOGFMT_ERROR(g_logger, "Error opening one session, exit!!!");
        exit(1);
    }
    LOGFMT_INFO(g_logger, "<%lx> start bampMatchThread...", pthread_self());
    //discharged when it is full and has none of new data.
    while(true){
        clockoutput_start("one circle of bampMatch");
        sleep(1);
        vector<BampMatchParam> allBufs;
        allBufs.clear();
        unsigned allcnt = 0;
        unsigned retcnt = 0;
        while(true){
            ProjectBuffer* tmpPrj = getNextBampProj();
            if(tmpPrj == NULL) break;
            allcnt ++;
            BampMatchParam tmpPar(tmpPrj->ID, tmpPrj);
            bool bPrjFull = false;
            tmpPrj->getUnBampData(tmpPar.preIdx, tmpPar.preOffset, tmpPar.endIdx, tmpPar.endOffset, tmpPar.data, tmpPar.bPreHit,  bPrjFull, tmpPar.curtime);
            if(tmpPar.data.size() == 0){
                if(bPrjFull){
                    delBampProj(tmpPrj);
                    retcnt ++;
                }
                continue;
            }
            tmpPar.preLen = tmpPar.data[0].getCap() * (tmpPar.preIdx - 1) + tmpPar.preOffset;
            tmpPar.tolLen = 0;
            for(size_t jdx=0; jdx < tmpPar.data.size(); jdx++){
                tmpPar.tolLen += tmpPar.data[jdx].len;
            }
            assert(tmpPar.tolLen % g_uBampFixedLen == 0);
            allBufs.push_back(tmpPar);
        }
        LOGFMT_DEBUG(g_logger, "one loop of bamp match, BampProjectNum: %u; ReturnBampProjectNum: %u; BampTaskNum: %lu.", allcnt, retcnt, allBufs.size());
        if(allBufs.size() > 0){
            obj->bamp_match_vad(allBufs);
            for(size_t idx=0; idx < allBufs.size(); idx++){
                BampMatchParam &par = allBufs[idx];
                par.ptrBuf->setBampEndPos(par.preIdx, par.preOffset, par.endIdx, par.endOffset, par.bHit);
                //append reported file here.
                appendDataToReportFile(par);
            }
        }
        string output = clockoutput_end();
        LOGFMTT(output.c_str());
    }
    return NULL;
}



int CloseDLL()
{
    if(g_bUseBamp){
        if(!bamp_rlse()){
            LOG_ERROR(g_logger, "fail to release bamp engine.");
            return 1;
        }
    }
    FreeVADCluster();
    //ioareg_rlse();
    rlse_bufferglobal();
    return 0;
}

int AddCfg(unsigned int id, 
        const char *strName,
        const char *strConfigFile,
        int iType,
        int iHarmLevel)
{
    return 0;
}
//TODO: make sure none of parallel unit are in spk recognition.
int AddCfgByBuf(const char *pData,
        int iDataBytes,
        unsigned int id,
        int iType,
        int iHarmLevel)
{
    return 0;
}

int AddCfgByDir(int iType, const char *strDir)
{
    return 0;
}
int RemoveAllCfg(int iType)
{
    return 0;
}
int RemoveCfgByID(unsigned int id, int iType, int iHarmLevel)
{
    return 0;
}

bool SetRecord(int iType, bool bRecord)
{
    LOG_INFO(g_logger, "SetRecord iType="<< iType<< " bRecord="<< bRecord<< " unimplemented.");
   return true;
}
int GetDataNum4Process(int iType[], int num[])
{
    LOG_INFO(g_logger, "GetDataNum4Process unimplemented.");
    return 0;
}

extern unsigned char linear2alaw(short pcm_val);
bool saveWaveAsAlaw(char* pcmData, unsigned pcmlen, const char *filePath)
{
    FILE *fp = fopen(filePath, "wb");
    if(fp == NULL) return false;
    char zero = '\0';
    for(size_t idx=0; idx < 256; idx++){
        fwrite(&zero, 1, 1, fp);
    }
    string alawBuf;
    alawBuf.resize(pcmlen / 2);
    char *tmpAlaw = const_cast<char*>(alawBuf.c_str());
    short *tmpPcm = reinterpret_cast<short*>(pcmData);
    for(size_t idx=0; idx < pcmlen /2; idx ++) tmpAlaw[idx] = linear2alaw(tmpPcm[idx]);
    fwrite(tmpAlaw, 1, pcmlen /2, fp);
    fclose(fp);
    return true;
}

bool saveWaveAsAlaw(FILE *fp, vector<DataBlock> &blks)
{
    unsigned len = 0;
    for(size_t idx=0; idx < blks.size(); idx++){
        DataBlock& curblk = blks[idx];
        len += curblk.len;
    }
    if(len == 0){ return true; }
    char * tmpAlaw = (char*)malloc(len / 2);
    unsigned nowIdx = 0;
    for(size_t idx=0; idx < blks.size(); idx++){
        DataBlock& curblk = blks[idx];
        short *tmppcm = reinterpret_cast<short*>(curblk.getPtr() + curblk.offset);
        for(size_t jdx=0; jdx < curblk.len / 2; jdx ++){
            tmpAlaw[nowIdx ++] = linear2alaw(tmppcm[jdx]);
        }
    }
    assert(nowIdx = len / 2);
    size_t retw = fwrite(tmpAlaw, 1, nowIdx, fp);
    if(retw != nowIdx){
        LOGFMT_ERROR(g_logger, "failed write data to file, size=%lu, written=%lu error: %s", nowIdx, retw, strerror(errno));
    }
    free(tmpAlaw);
    if(retw != nowIdx) return false;
    return true;
}

static char g_sz256Zero[256];
void reportBampResultSeg(BampResultParam prm, ostream &oss)
{
    CDLLResult *pResult = prm.pResult;
    struct timeval curtime = prm.curtime;
    //vector<DataBlock> & vecData = prm.data;
    oss<<  " CfgID="<< pResult->m_iTargetID <<" InProjectStart="<< pResult->m_fSegPosInPCB[0]<< " MatchedDuration="<< pResult->m_fTargetMatchLen<<" InCfgStart="<< pResult->m_fSegPosInTarget[0]<< " MatchRate="<< pResult->m_fSegLikely[0];
    char savedfile[MAX_PATH];
    savedfile[0] = '\0';
    unsigned short type = pResult->m_iAlarmType;
    const WavDataUnit &prj = (*pResult->m_pDataUnit[0]);
    
    unsigned cfgId = pResult->m_iTargetID;
    int cfgScore = pResult->m_fSegLikely[0];
#if 1
    gen_spk_save_file(savedfile, m_TSI_SaveTopDir, NULL, curtime.tv_sec, prj.m_iPCBID, &type, &cfgId, &cfgScore);
    char * stSufPtr = strrchr(savedfile, '.');
    sprintf(stSufPtr, "_%.2f_%.2f.wav", pResult->m_fSegPosInPCB[0], pResult->m_fSegPosInTarget[0]);
    if(!saveWave(prj.m_pData, prj.m_iDataLen, savedfile)){
        LOGFMT_ERROR(g_logger, "reportBampResultSeg failed to write bamp pointed wave segment to file %s.", savedfile);
    }
    else{
        oss<<" SegmentPath="<< savedfile;
    }
    
    gen_spk_save_file(savedfile, m_TSI_SaveTopDir, NULL, curtime.tv_sec, prj.m_iPCBID, NULL, NULL, NULL);
    stSufPtr = strrchr(savedfile, '.');
    strcpy(stSufPtr+1, "pcm");
    if(!prm.bPreHit){
        FILE* fp = fopen(savedfile, "wb");
        if(fp == NULL){
            LOG_ERROR(g_logger, "failed to open bamp report file, file: "<< savedfile<< " error: "<< strerror(errno));
        }
        else{
            fwrite(g_sz256Zero, 256, 1, fp);
            fclose(fp);
        }
    }
    snprintf(pResult->m_strInfo, 1024, "%s:%s", g_strIp.c_str(), savedfile);
    oss<< " ReportPath="<< pResult->m_strInfo;
#endif
    g_ReportResultAddr(g_iModuleID, pResult);
}

bool reportIoacasResult(CDLLResult &result, char *writeLog, unsigned &len)
{
    unsigned long &pid = result.m_pDataUnit[0]->m_iPCBID;
    int confidence = (int)result.m_fLikely;
    unsigned configID = result.m_iTargetID;
    unsigned short alarmType = result.m_iAlarmType;
    len += sprintf(writeLog + len, "ALARMTYPE=%u TARGETID=%u CONFIDENCE=%d ", alarmType, configID, confidence);
    char savedfile[MAX_PATH];
    time_t cur_time;
    time(&cur_time);
    gen_spk_save_file(savedfile, m_TSI_SaveTopDir, NULL, cur_time, pid, &alarmType, &configID, &confidence);
    snprintf(result.m_strInfo, 1024, "%s:%s", g_strIp.c_str(), savedfile);
    bool retSave = saveWave((char*)result.m_pDataUnit[0]->m_pData, result.m_pDataUnit[0]->m_iDataLen, savedfile);
    if(retSave){
        if(writeLog!=NULL){
            len += sprintf(writeLog + len, "DATASAVEPATH=%s ", result.m_strInfo);
        }
    }
    
    bool brep = false;
    if(g_mLangReportFilter.find(make_pair(result.m_iAlarmType, result.m_iTargetID)) != g_mLangReportFilter.end() && g_mLangReportFilter[make_pair(result.m_iAlarmType, result.m_iTargetID)] <= result.m_fLikely){
        brep = true;
    }
    if(brep){
        result.m_iTargetID |= 0x200;
        g_ReportResultAddr(g_iModuleID, &result);
        result.m_iTargetID &= ~0x200;
        const char* debugDir = "ioacas/debug/";
        if(if_directory_exists(debugDir)){
            char wholePath[MAX_PATH];
            sprintf(wholePath, "%sMessage_%lu", debugDir, pid);
            save_binary_data(wholePath, &result, sizeof(CDLLResult), result.m_pDataUnit[0], sizeof(WavDataUnit), NULL);
        }
    }
    return true;
}

void *ioacas_maintain_procedure(void *)
{
    while(true){

        time_t cur_time;
        time(&cur_time);

        if(true){
            //update some configurations.
            static time_t lasttime;
            if(cur_time > 3 + lasttime){
                lasttime = cur_time;
                if(g_AutoCfg.checkAndLoad()){
                }
            }
        }
        //ioareg_maintain_procedure(cur_time);
        sleep(3);
    }
    return NULL;
}


static char g_szAllPrjsDir[MAX_PATH]; //save all projects
static pthread_mutex_t g_AllProjsDirLock = PTHREAD_MUTEX_INITIALIZER;
void *dummyRecThread(void *)
{
    ProjectBuffer *ptrBuf = NULL;
    while(true){
        ptrBuf = obtainFullBufferTimeout(-1u);
        if (ptrBuf != NULL)
        {
            struct timeval cur_time = ptrBuf->getPrjTime();
            ptrBuf->startMainReg();
            vector<DataBlock> prjData;
            ptrBuf->getData(prjData);
            //save all projects, mainly for debug.
            pthread_mutex_lock(&g_AllProjsDirLock);
            size_t tmpLen = strlen(g_szAllPrjsDir);
            if(tmpLen > 0 && if_directory_exists(g_szAllPrjsDir)){
                char savedfile[MAX_PATH];
                gen_spk_save_file(savedfile, g_szAllPrjsDir, NULL, cur_time.tv_sec, ptrBuf->ID, NULL, NULL, NULL);
                pthread_mutex_unlock(&g_AllProjsDirLock);
                if(!saveWave(prjData, savedfile)){
                }
            }
            else{
                pthread_mutex_unlock(&g_AllProjsDirLock);
            }
            ptrBuf->finishMainReg();
            returnBuffer(ptrBuf);
        }
    }

    return NULL;
}



extern "C"
{
void _ZSt24__throw_out_of_range_fmtPKcz()
{
    
}
}
