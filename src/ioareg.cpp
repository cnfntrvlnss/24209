/*************************************************************************
	> File Name: ioareg.cpp
	> Author: 
	> Mail: 
	> Created Time: Tue 22 Nov 2016 03:58:09 AM EST
 ************************************************************************/

#include "ioareg.h"

#include<iostream>
#include <map>
#include <queue>
#include <string>
using namespace std;

#include "MusicDetect.h"
#include "dllVAD_dup.h"
#include "TLI_API_dup.h"
#include "dllSRVADCluster.h"

struct RecogThreadSpace{
    RecogThreadSpace(){
        threadIdx = 0;
        result.m_iDataUnitNum = 1;
        result.m_pDataUnit[0] = &projData;
        accntTotalCnt = 0;
        accntTotalSecs = 0;
    }
    ~RecogThreadSpace(){
    }
    pthread_t threadId;
    unsigned threadIdx;
    struct timeval curTime;
    //drain variable in the sequence of recognization.
    CDLLResult result;
    WavDataUnit projData;
    short *vadBuf;
    unsigned vadBufLen;
    short *mcutBuf;
    unsigned mcutBufLen;
    unsigned long accntTotalCnt;
    unsigned long accntTotalSecs;
    LockHelper lock;
    void addAccnt(){
        lock.lock();
        accntTotalCnt ++;
        accntTotalSecs += projData.m_iDataLen / 16000;
        lock.unLock();
    }
    void getAccnt(unsigned long& prjcnt, unsigned long& bytecnt){
        lock.lock();
        prjcnt = accntTotalCnt;
        bytecnt = accntTotalSecs;
        lock.unLock();
    }
private:
    RecogThreadSpace(const RecogThreadSpace&);
    RecogThreadSpace& operator=(const RecogThreadSpace&);
};

char g_szAllPrjsDir[MAX_PATH]; //save all projects, when after processing.
pthread_mutex_t g_AllProjsDirLock = PTHREAD_MUTEX_INITIALIZER;
static pthread_key_t g_RecWavBufsKey;
//static pthread_t *g_pthread_id = NULL;
static short g_ThreadNum = 1;
static RecogThreadSpace *g_RecSpaceArr = NULL;
LoggerId g_StatusLogger;

//////////////////-----vad-----
static bool g_bUseVAD=false;
static int g_iVADPrecent = 101;
static pthread_mutex_t g_VADPrecentLock = PTHREAD_MUTEX_INITIALIZER;
//////////////////----music----
static bool g_bUseMusicDetect=false;
static int g_iMusicPrecent = 101;
static pthread_mutex_t g_MusicPrecentLock = PTHREAD_MUTEX_INITIALIZER;
static char szMusicDetectCfg[] = "./ioacas/Music.cfg";
///////////////////---lid---
static bool g_bUseLid=true;
static char szLIDVADCfg[MAX_PATH] = "./ioacas/VAD_LID.cfg";
static char szLIDCfgDir[MAX_PATH] = "./ioacas/sysdir";
void* IoaRegThread(void *param);

///////////////////---spk---
static char szSpkMscCfg[MAX_PATH] = "./ioacas/Music.cfg";
static char szSpkVADCfg[MAX_PATH] = "./ioacas/VAD_SID.cfg";
static char szSpkCfgFile[MAX_PATH] = "./ioacas/SpkSRE.cfg";
bool g_bUseSpk = true;
static bool g_bSpkUseVad = false;
static bool g_bSpkUseMCut = false;
static float defaultSpkScoreThrd;
//static queue<pair<const SpkInfoChd*, map<pthread_t, unsigned long> > > g_PendingDeleteSpks;

bool SpkInfoChd::fromStr(const char* strSpk){
    istringstream iss(strSpk);
    char chSep;
    if(!(iss >> this->spkId)) return false;
    if(!(iss.get(chSep) || chSep != '_')) return false;
    int retval = iss.peek();
    if(retval == EOF) return false;
    int servtype;
    if(retval == '0'){
        char tmpch;
        iss.get(tmpch);
        iss.get(tmpch);
        if(tmpch != 'x' && tmpch != 'X') return false;
        if(!(iss >> std::hex>> servtype>> std::dec)) return false;
    }
    else{
        if(!(iss >> servtype)) return false;
    }
    this->servType = servtype;
    if(!(iss.get(chSep) || chSep != '_')) return false;
    if(!(iss >> harmLevel)) return false;
    return true;
}

bool addSpkPerFile(const char* szDir, const char* filename)
{
    SpkInfoChd *spk = new SpkInfoChd();
    if(!spk->fromStr(filename)){
        delete spk;
        return false;
    }
    if(spk->servType != 0x92){
        delete spk;
        return false;
    }
    string filePath = concatePath(szDir, filename);
    FILE *fp = fopen(filePath.c_str(), "rb");
    if(fp == NULL){
        LOGFMT_ERROR(g_logger, "in addSpkPerFile, failed to open file: %s", filePath.c_str());
        delete spk;
        return false;
    }
    char *mdlData = NULL;
    unsigned mdlLen = 0;
    fseek(fp, 0, SEEK_END);
    mdlLen = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    mdlData = new char[mdlLen];
    size_t retr = fread(mdlData, 1, mdlLen, fp);
    bool ret = false;
    if(retr == mdlLen){
        if(!spkex_addSpk(spk, mdlData, mdlLen)){
            LOGFMT_ERROR(g_logger, "in addSpkPerFile, failed in spkex_addSpk, filepath: %s", filePath.c_str());
        }
        else{
            ret = true;
        }
    }
    else{
        LOGFMT_ERROR(g_logger, "in addSpkPerFile, failed to read file: %s, real: %u; read: %u.", filePath.c_str(), mdlLen, retr);
    }
    delete []mdlData;
    if(!ret){
        delete spk;
    }
    return ret;
}

typedef struct{
	float m_0Value;
	float  m_100Value;
	float m_maxValue;// in term of original score.
} ScoreConfig;
typedef float (*TransScore)(float);

static ScoreConfig g_cfg;
static char g_SCinit = 0;
static float trans_score(float f)
{
	if(f > g_cfg.m_maxValue) f = g_cfg.m_maxValue;
	return (f - g_cfg.m_0Value) * ((100) / (g_cfg.m_100Value - g_cfg.m_0Value));
}
TransScore getScoreFunc(ScoreConfig *param)
{
	if(param == NULL && g_SCinit == 0){
		g_cfg.m_0Value = 0.0;
		g_cfg.m_100Value = 100.0;
		g_cfg.m_maxValue = 100.0;
	}
	else if(param != NULL){
		g_SCinit = 1;
		g_cfg = *param;
        g_cfg.m_maxValue = g_cfg.m_100Value;
	}
	return trans_score;
}


static void ioareg_updateConfig()
{
    if(g_AutoCfg.isUpdated("lid", "savedMusicPrecent")){
        pthread_mutex_lock(&g_MusicPrecentLock);
        Config_getValue(&g_AutoCfg, "lid", "savedMusicPrecent", g_iMusicPrecent);
        pthread_mutex_unlock(&g_MusicPrecentLock);
    }
    if(g_AutoCfg.isUpdated("lid", "savedVadPrecent")){
        pthread_mutex_lock(&g_VADPrecentLock);
        Config_getValue(&g_AutoCfg, "lid", "savedVadPrecent", g_iVADPrecent);
        pthread_mutex_unlock(&g_VADPrecentLock);
    }
    if(g_AutoCfg.isUpdated("", "saveAllTopDir")){
        pthread_mutex_lock(&g_AllProjsDirLock);
        Config_getValue(&g_AutoCfg, "", "saveAllTopDir", g_szAllPrjsDir);
        unsigned tmpLen = strlen(g_szAllPrjsDir);
        if(tmpLen > 0 && g_szAllPrjsDir[tmpLen - 1] != '/'){
            g_szAllPrjsDir[tmpLen] = '/';
            g_szAllPrjsDir[tmpLen + 1] = '\0';
        }
        pthread_mutex_lock(&g_AllProjsDirLock);
    }
}


static void operate_spklib_offline()
{
	string transdir = "ioacas/SpkModelTrans/";
	string taskfile = transdir + "tasklist";
	FILE * taskfd = fopen(taskfile.c_str(), "r");
	if(taskfd == NULL){
		return ;
	}
    LOGFMT_INFO(g_logger, "operate_spklib_offline begin process new task.");
	char tmpline[512];
	char *taskstr, *modelname;
	while(fgets(tmpline, 512, taskfd) != NULL){
		taskstr = strtok(tmpline, " \t\n");
		modelname = strtok(NULL, " \t\n");
		if(taskstr == NULL || modelname == NULL) {
			LOGFMT_WARN(g_logger, "operate_spklib_offline fails to parse task in %s", tmpline);
			continue;
		}
		if(strcmp(taskstr, "add")==0){
			//parse information of the added speaker from the 2 and 3 tokens.
			const char *thdstr = strtok(NULL, " \t\n");
            if(thdstr != NULL){
                 float thdf = atof(thdstr);   
            }
            SpkInfoChd* curspk = new SpkInfoChd();
            if(!curspk->fromStr(modelname)){
                delete curspk;
				LOGFMT_WARN(g_logger, "operate_spklib_offline failed to parse modelname in %s", tmpline);
                continue;
            }
			//read data from model file specified by the 2th token.
			string modelfile = transdir + modelname;
			FILE *mdfd = fopen(modelfile.c_str(), "rb");
			if(mdfd == NULL){
				LOGFMT_WARN(g_logger, "operate_spklib_offline failed to open file %s. errno: %s", modelfile.c_str(), strerror(errno));
                delete curspk;
				continue;
			}
			fseek(mdfd, 0, SEEK_END);
			size_t modellen = ftell(mdfd);
			fseek(mdfd, 0, SEEK_SET);
            if(modellen == 0){
                LOGFMT_WARN(g_logger, "operate_spklib_offline empty sample file %s.", modelfile.c_str());
                delete curspk;
                fclose(mdfd);
                continue;
            }
			char *modeldata = (char*)malloc(modellen);
			int retr = fread(modeldata, 1, modellen, mdfd);
            if(!spkex_addSpk(curspk, modeldata, modellen)){
                delete curspk;
            }
			free(modeldata);
			fclose(mdfd);
		}
		else if(strcmp(taskstr, "remove") == 0){
			unsigned speakid;
			if(sscanf(modelname, "%u", &speakid) < 1){
				LOGFMT_WARN(g_logger, "operate_spklib_offline failed to parse spkid in %s.", tmpline);
				continue;
			}
            spkex_rmSpk(speakid);
		}
	}
	int retrm = remove(taskfile.c_str());
	if(retrm == -1){
		LOGFMT_WARN(g_logger, "operate_spklib_offline failed to remove %s error: %s", taskfile.c_str(), strerror(errno));
	}
}

void ioareg_maintain_procedure(time_t curTime)
{
    static time_t lasttime;
    if(curTime > 3 + lasttime){
        lasttime = curTime;
        if(g_AutoCfg.checkAndLoad()){
            ioareg_updateConfig();
        }
    }
    static time_t statuslasttime;
    if(curTime > 30 + statuslasttime){
        statuslasttime = curTime;
        char sztable[1024];
        unsigned curlen = 0;
        curlen += sprintf(sztable + curlen, "");
        for(short idx=0; idx < g_ThreadNum; idx++){
            unsigned long totalcnt, totalsecs;
            g_RecSpaceArr[idx].getAccnt(totalcnt, totalsecs);
            curlen += sprintf(sztable + curlen, "threadIdx: %d    totalcnt: %u    totalsecs: %u\n", idx, totalcnt, totalsecs);
        }
        LOGFMT_INFO(g_StatusLogger, "*************ioareg status*************\n%s", sztable);
        string statStr;
        getBufferStatus(statStr);
        LOGFMT_INFO(g_StatusLogger, "*************projectbuffer status*************\n%s", statStr.c_str());
    }
    static time_t offlinelasttime;
    if(curTime > 3 + offlinelasttime){
        offlinelasttime = curTime;
        operate_spklib_offline();
    }
}

bool ioareg_init()
{
	ScoreConfig ssCfg;
    ssCfg.m_0Value = 0;
	ssCfg.m_100Value = 100;
    Config_getValue(&g_AutoCfg, "", "saveAllTopDir", g_szAllPrjsDir);
    Config_getValue(&g_AutoCfg, "", "saveDebugTopDir", g_szDebugBinaryDir);
    Config_getValue(&g_AutoCfg, "", "ifUseVAD", g_bUseVAD);
    Config_getValue(&g_AutoCfg, "", "ifUseMusicDetect", g_bUseMusicDetect);
    Config_getValue(&g_AutoCfg, "", "savedMusicPrecent", g_iMusicPrecent);
    Config_getValue(&g_AutoCfg, "", "savedVadPrecent", g_iVADPrecent);
    Config_getValue(&g_AutoCfg, "", "threadNumPerStream", g_ThreadNum);
    Config_getValue(&g_AutoCfg, "lid","ifUseLID", g_bUseLid);
    Config_getValue(&g_AutoCfg, "spk", "ifUseSPK", g_bUseSpk);
    Config_getValue(&g_AutoCfg, "spk", "ifUseVAD", g_bSpkUseVad);
    Config_getValue(&g_AutoCfg, "spk", "ifUseMusicDetect", g_bSpkUseMCut);
    Config_getValue(&g_AutoCfg, "spk", "zeroScore", ssCfg.m_0Value);
    Config_getValue(&g_AutoCfg, "spk", "hundredScore", ssCfg.m_100Value);
    Config_getValue(&g_AutoCfg, "spk", "defaultThreshold", defaultSpkScoreThrd);
    unsigned tmpLen = strlen(g_szAllPrjsDir);
	if(tmpLen > 0 && g_szAllPrjsDir[tmpLen - 1] != '/'){
		g_szAllPrjsDir[tmpLen] = '/';
		g_szAllPrjsDir[tmpLen + 1] = '\0';
	}

	g_StatusLogger = g_Log4zManager->createLogger("status");
#define LOG4Z_VAR(x) << #x "=" << x << "\n"
    LOG_INFO(g_logger, "====================config====================\n" 
            LOG4Z_VAR(g_szAllPrjsDir)
            LOG4Z_VAR(g_szDebugBinaryDir)
            LOG4Z_VAR(g_ThreadNum)
            LOG4Z_VAR(g_bUseVAD)
            LOG4Z_VAR(g_bUseLid)
            LOG4Z_VAR(g_bUseMusicDetect)
            LOG4Z_VAR(g_iVADPrecent)
            LOG4Z_VAR(szMusicDetectCfg)
            LOG4Z_VAR(szLIDCfgDir)
            LOG4Z_VAR(g_bUseSpk)
            LOG4Z_VAR(g_bSpkUseVad)
            LOG4Z_VAR(g_bSpkUseMCut)
            LOG4Z_VAR(g_iMusicPrecent)
            LOG4Z_VAR(defaultSpkScoreThrd)
            LOG4Z_VAR(ssCfg.m_0Value)
            LOG4Z_VAR(ssCfg.m_100Value)
            );

    int err = pthread_key_create(&g_RecWavBufsKey, free);
    if(err != 0){
        LOG_ERROR(g_logger, "fail to create pthread key, error: "<< err);
        return false;
    }

    if(g_bUseSpk){
        if(!spkex_init(szSpkCfgFile)){
            LOG_ERROR(g_logger, "fail to initialize spk engine.");
            return false;
        }
        else{
            LOG_INFO(g_logger, "finish initializing spk engine.");
        }
        getScoreFunc(&ssCfg);
    }
    if(g_bSpkUseVad){
        if(!InitVADCluster(szSpkVADCfg)){
            LOG_ERROR(g_logger, "fail to initialize vad cluster engine.");
            return false;
        }
        LOG_INFO(g_logger, "finish initializing vad cluster engine.");
    }
if(g_bSpkUseMCut){
	if(!MusicCut_Initial(szSpkMscCfg, g_ThreadNum)){
	LOG_ERROR(g_logger, "fail to initail music engine.");
	return false;
	}
}

    initLID(g_ThreadNum);
	//g_pthread_id = (pthread_t *)malloc(sizeof(pthread_t) * (g_ThreadNum));

    g_RecSpaceArr = new RecogThreadSpace[g_ThreadNum];
    for (int i = 0; i < g_ThreadNum; ++i)
	{
		pthread_attr_t threadAttr;
		pthread_attr_init(&threadAttr);
        //TODO why specify stack size.
		pthread_attr_setstacksize(&threadAttr, 2080 * 1024); // 120*1024
		//pthread_attr_setdetachstate(&threadAttr, PTHREAD_CREATE_DETACHED);
        g_RecSpaceArr[i].threadIdx = i;
		int retc = pthread_create(&g_RecSpaceArr[i].threadId, &threadAttr, IoaRegThread, &g_RecSpaceArr[i]);
		if(retc != 0){
			LOG_ERROR(g_logger, "fail to create recognition thread, index "<< g_RecSpaceArr[i].threadIdx);
            exit(1);
		}
		pthread_attr_destroy(&threadAttr);
	}

/*
string g_strExtraSpkModelDir = "ioacas/OrigSpkModel";
if(if_directory_exists(g_strExtraSpkModelDir.c_str())){
	int retspkcfg = procFilesInDir(g_strExtraSpkModelDir.c_str(), addSpkPerFile);
	LOGFMT_INFO(g_logger, "in ioareg_init, loading prepared spk models in %s, SPKCount=%d.", g_strExtraSpkModelDir.c_str(), retspkcfg);
}
*/

    return true;
}

bool ioareg_rlse()
{
    LOG_INFO(g_logger, "starting shutdown ioacas.");
	for(int i = 0; i < g_ThreadNum; i++){
		pthread_cancel(g_RecSpaceArr[i].threadId);
	}
    for(int idx=0; idx < g_ThreadNum; idx++){
        pthread_join(g_RecSpaceArr[idx].threadId, NULL);
    }

    if(g_bUseSpk) spkex_rlse();
    if(g_bSpkUseVad) FreeVADCluster();
    if(g_bSpkUseMCut) MusicCut_Free();

    delete []g_RecSpaceArr;
    g_RecSpaceArr = NULL;
    return true;
}

static inline bool  checkAndSetLidResSt(CDLLResult &res, int nMax, float score)
{
    bool ret = false;
    for(size_t s = 0; s< g_mLangReports.size(); s++){
        if(nMax == g_mLangReports[s].first){
            int tmpscr = score < 1.0001? (int)(score * 100): (int)score;
            res.m_iAlarmType = g_mLangReports[s].second.first;
            res.m_iTargetID = g_mLangReports[s].second.second;
            res.m_iHarmLevel = 0;
            res.m_fLikely = tmpscr;
            res.m_fSegLikely[0] = tmpscr;
            return true;
        }
    }
    return ret;

}

static void vadProcess(RecogThreadSpace &rec, int hVad)
{
    char* &pData = rec.projData.m_pData;
    unsigned &dataLen = rec.projData.m_iDataLen;
    unsigned long &pid = rec.projData.m_iPCBID;
    short *recBuf = reinterpret_cast<short*>(pData);
    unsigned recBufLen = dataLen / sizeof(short);
	unsigned logLen = 0;
	char WriteLog[1024];
    logLen = 0;
    logLen += sprintf(WriteLog+logLen, "VADREG PID=%lu WavLen=%us ", pid, dataLen / PCM_ONESEC_LEN);
    while(true){
        if(recBufLen < POSITIVE_PCM_LEN)
        {
            sprintf(WriteLog+strlen(WriteLog), "too short ");
            rec.mcutBufLen = 0;
            break;
        }
        bool retv = cutVAD(hVad, recBuf, recBufLen, rec.vadBuf, rec.vadBufLen);
        if(retv){
        }
        else{
            rec.vadBufLen = 0;   
        }
        logLen += sprintf(WriteLog + logLen, "VADCutLen=%ds ", rec.vadBufLen / PCM_ONESEC_SMPS);
        int vadRatio = (1 - (float)rec.vadBufLen / recBufLen) * 100;
        pthread_mutex_lock(&g_VADPrecentLock);
        int ivprecent = g_iVADPrecent;
        pthread_mutex_unlock(&g_VADPrecentLock);
        if(vadRatio > ivprecent){
            rec.result.m_iTargetID = g_uVADID;
            rec.result.m_iAlarmType = g_uVADType;
            rec.result.m_iHarmLevel = 0;
            rec.result.m_fLikely = vadRatio;
            rec.result.m_fSegLikely[0] = vadRatio;
            reportIoacasResult(rec.result, WriteLog, logLen);
        }
        recBuf = rec.vadBuf;
        recBufLen = rec.vadBufLen;
        break;
    }
    LOG_INFO(g_logger, WriteLog<< "eop.");

}

static void musicProcess(RecogThreadSpace &rec)
{
    char* &pData = rec.projData.m_pData;
    unsigned &dataLen = rec.projData.m_iDataLen;
    unsigned long &pid = rec.projData.m_iPCBID;
    short *recBuf = reinterpret_cast<short*>(pData);
    unsigned recBufLen = dataLen / sizeof(short);
	unsigned logLen = 0;
	char WriteLog[1024];
    logLen = 0;
    logLen += sprintf(WriteLog+logLen, "MSCREG PID=%lu WavLen=%us ", pid, dataLen / PCM_ONESEC_LEN);
    while(true){
        if(recBufLen < POSITIVE_PCM_LEN)
        {
            sprintf(WriteLog+strlen(WriteLog), "too short ");
            rec.mcutBufLen = 0;
            break;
        }

        int tmpVar = recBufLen;
        bool retm = MusicCut(rec.threadIdx, recBuf, recBufLen, rec.mcutBuf, tmpVar);
        rec.mcutBufLen = tmpVar;
        if(retm){
        }
        else{
            rec.mcutBufLen = 0;
        }
        logLen += sprintf(WriteLog + logLen, "MusicCutLen=%ds ", rec.mcutBufLen / PCM_ONESEC_SMPS);
        int  mscRatio = (1 - (float)rec.mcutBufLen / recBufLen) * 100; 
        pthread_mutex_lock(&g_MusicPrecentLock);
        int imprecent = g_iMusicPrecent;
        pthread_mutex_unlock(&g_MusicPrecentLock);
        if(imprecent < mscRatio){
            rec.result.m_iTargetID = g_uMusicID;
            rec.result.m_iAlarmType = g_uMusicType;
            rec.result.m_iHarmLevel = 0;
            rec.result.m_fLikely = mscRatio;
            rec.result.m_fSegLikely[0] = mscRatio;
            reportIoacasResult(rec.result, WriteLog, logLen);
        }
        break;
    }
    
    LOG_INFO(g_logger, WriteLog<< "eop.");
}

static void lidRegProcess(RecogThreadSpace &rec, int hTLI)
{
    char* &pData = rec.projData.m_pData;
    unsigned &dataLen = rec.projData.m_iDataLen;
    unsigned long &pid = rec.projData.m_iPCBID;
	unsigned logLen = 0;
	char WriteLog[1024];
    logLen = 0;
    logLen += sprintf(WriteLog+logLen, "LIDREG PID=%lu WavLen=%us ", pid, dataLen / PCM_ONESEC_LEN);
    //clockoutput_start("LIDREG TIME CONSUMING ");
    clockoutput_start("CLOCK_RECORD %s", WriteLog);
string timelid;
    while(true){
        short *recBuf = reinterpret_cast<short*>(pData);
        unsigned recBufLen = dataLen / sizeof(short);
        if(recBufLen < POSITIVE_PCM_LEN)
        {
            sprintf(WriteLog+strlen(WriteLog), "too short ");
            rec.mcutBufLen = 0;
            rec.vadBufLen = 0;
            break;
        }
        /*
        if(hMCut != NULL){
            set_count();
            bool retm = cutMusic(hMCut, recBuf, recBufLen, rec.mcutBuf, rec.mcutBufLen);
            if(retm){
            }
            else{
                rec.mcutBufLen = 0;
            }
            logLen += sprintf(WriteLog + logLen, "MusicCutLen=%ds ", rec.mcutBufLen / PCM_ONESEC_SMPS);
            int  mscRatio = (1 - (float)rec.mcutBufLen / recBufLen) * 100; 
            pthread_mutex_lock(&g_MusicPrecentLock);
            int imprecent = g_iMusicPrecent;
            pthread_mutex_unlock(&g_MusicPrecentLock);
            if(imprecent < mscRatio){
                rec.result.m_iTargetID = g_uMusicID;
                rec.result.m_iAlarmType = g_uMusicType;
                rec.result.m_iHarmLevel = 0;
                rec.result.m_fLikely = mscRatio;
                rec.result.m_fSegLikely[0] = mscRatio;
                reportIoacasResult(rec.result, WriteLog, logLen);
            }
            recBuf = rec.mcutBuf;
            recBufLen = rec.mcutBufLen;
        }

        if(recBufLen < POSITIVE_PCM_LEN)
        {
            sprintf(WriteLog+strlen(WriteLog), "too short ");
            rec.vadBufLen = 0;
            break;
        }

        if(hVAD != -1){
            set_count();
            bool retv = cutVAD(hVAD, recBuf, recBufLen, rec.vadBuf, rec.vadBufLen);
            if(retv){
            }
            else{
                rec.vadBufLen = 0;   
            }
            logLen += sprintf(WriteLog + logLen, "VADCutLen=%ds ", rec.vadBufLen / PCM_ONESEC_SMPS);
            int vadRatio = (1 - (float)rec.vadBufLen / recBufLen) * 100;
            pthread_mutex_lock(&g_VADPrecentLock);
            int ivprecent = g_iVADPrecent;
            pthread_mutex_unlock(&g_VADPrecentLock);
            if(vadRatio > ivprecent){
                rec.result.m_iTargetID = g_uVADID;
                rec.result.m_iAlarmType = g_uVADType;
                rec.result.m_iHarmLevel = 0;
                rec.result.m_fLikely = vadRatio;
                rec.result.m_fSegLikely[0] = vadRatio;
                reportIoacasResult(rec.result, WriteLog, logLen);
            }
            recBuf = rec.vadBuf;
            recBufLen = rec.vadBufLen;
        }

        if(recBufLen < POSITIVE_PCM_LEN)
        {
            sprintf(WriteLog+strlen(WriteLog), "too short ");
            break;
        }
        */
        int nMax;
        float score;
        LOG_TRACE(g_logger, "LIDREG before scoreTLI, save raw pcm in "<< saveTempBinaryData(rec.curTime, pid, reinterpret_cast<char*>(recBuf), recBufLen * sizeof(short)));
clockoutput_start("ScoreLID RecLen %u", recBufLen);
        scoreTLI_dup(hTLI, recBuf, recBufLen, nMax, score);
timelid = clockoutput_end();
        if(checkAndSetLidResSt(rec.result, nMax, score)){
            reportIoacasResult(rec.result, WriteLog, logLen);
        }
        break;
    }
    LOGFMTD("%s %s", clockoutput_end().c_str(), timelid.c_str());
    LOG_INFO(g_logger, WriteLog<< "eop.");
}

static void spkRegProcess(RecogThreadSpace &rec)
{
    char* &pData = rec.projData.m_pData;
    unsigned &dataLen = rec.projData.m_iDataLen;
    unsigned long &pid = rec.projData.m_iPCBID;
	unsigned logLen = 0;
#define MAX_LINE 1024
	char WriteLog[MAX_LINE];
    logLen = 0;
    logLen += snprintf(WriteLog+logLen, MAX_LINE - logLen,  "<%lu> SPKREG PID=%lu WavLen=%us ", pthread_self(), pid, dataLen / PCM_ONESEC_LEN);
    clockoutput_start("CLOCK_RECORD %s", WriteLog);
string timemusic, timevad, timespk;
    while(true){
        short *recBuf = reinterpret_cast<short*>(pData);
        unsigned recBufLen = dataLen / sizeof(short);
        rec.mcutBufLen = 0;
        rec.vadBufLen = 0;
        if(recBufLen < POSITIVE_PCM_LEN)
        {
            logLen += snprintf(WriteLog+logLen, MAX_LINE - logLen, "too short ");
            break;
        }
        if(g_bSpkUseMCut){
            int tmpval = recBufLen;
            clockoutput_start("CutMusic %u", recBufLen);
            bool retmc = MusicCut(rec.threadIdx, recBuf, recBufLen, rec.mcutBuf, tmpval);
            timemusic = clockoutput_end();
            rec.mcutBufLen = tmpval;
            if(!retmc){ rec.mcutBufLen = 0; }
            recBuf = rec.mcutBuf;
            recBufLen = rec.mcutBufLen;
            logLen += snprintf(WriteLog + logLen, MAX_LINE - logLen, "MCutLen=%us ", recBufLen / PCM_ONESEC_SMPS);
            if(recBufLen < POSITIVE_PCM_LEN)
            {
                logLen += sprintf(WriteLog+logLen, "too short ");
                break;
            }
        }
        if(g_bSpkUseVad){
            int tmpval = recBufLen;
            clockoutput_start("CutVAD %u", recBufLen);
            bool retvad = VADBuffer(true, recBuf, recBufLen, rec.vadBuf, tmpval);
            timevad = clockoutput_end();
            rec.vadBufLen = tmpval;
            if(!retvad) rec.vadBuf = 0;
            recBuf = rec.vadBuf;
            recBufLen = rec.vadBufLen;
            logLen += snprintf(WriteLog + logLen, MAX_LINE - logLen, "VADLen=%us ", recBufLen / PCM_ONESEC_SMPS);
            if(recBufLen < POSITIVE_PCM_LEN)
            {
                logLen += sprintf(WriteLog+logLen, "too short ");
                break;
            }
        }
        const SpkInfo *spk;
        float score;
        LOG_TRACE(g_logger, "SPKREG before spkex_score, save raw pcm in "<< saveTempBinaryData(rec.curTime, pid, reinterpret_cast<char*>(recBuf), recBufLen * sizeof(short)));
clockoutput_start("ScoreSpk %u", recBufLen);
        int retspk = spkex_score(recBuf, recBufLen, spk, score);
timespk = clockoutput_end();
        if(spk == NULL){
            if(retspk == 0) logLen += snprintf(WriteLog + logLen, MAX_LINE - logLen, "no speaker sample ");
             break;
        }
        const SpkInfoChd *rspk = dynamic_cast<const SpkInfoChd*>(spk);
        CDLLResult &res = rec.result;
        res.m_iAlarmType = rspk->servType;
        res.m_iTargetID = rspk->spkId;
        res.m_iHarmLevel = rspk->harmLevel;
        res.m_fLikely = getScoreFunc(NULL)(score);
        res.m_fSegLikely[0] = res.m_fLikely;
        returnSpkInfo(spk);
        if(res.m_fLikely >=defaultSpkScoreThrd){
            reportIoacasResult(rec.result, WriteLog, logLen);
        }
        break;
    }
    LOGFMTT("%s %s %s %s", clockoutput_end().c_str(), timemusic.c_str(),timevad.c_str(), timespk.c_str());
    LOG_INFO(g_logger, WriteLog<< "eop.");
}

struct WavBuf{
    short *data;
    unsigned len;
};

static unsigned g_uRecWavBufsCount = 3;
static unsigned g_uWavBufLenMin = 40 * 60 * 8000;//30 minutes buffer for recognition. provided above %90 precents of tasks is of less length than it.

static inline WavBuf* getRecWavBufs()
{
    WavBuf* ret = (WavBuf*)pthread_getspecific(g_RecWavBufsKey);
    if(ret == NULL){
        ret = (WavBuf*)malloc(sizeof(WavBuf) * g_uRecWavBufsCount);
        for(unsigned idx=0; idx < g_uRecWavBufsCount; idx++){
            ret[idx].data = (short*)malloc(sizeof(short) * g_uWavBufLenMin);
            ret[idx].len = g_uWavBufLenMin;
        }
        pthread_setspecific(g_RecWavBufsKey, ret);
    }
    return ret;
}

static inline void release_rec_bufs(unsigned remLen = 0)
{
    WavBuf* localWavBufs = getRecWavBufs();
    if(remLen < g_uWavBufLenMin){
        remLen = g_uWavBufLenMin;
    }
    for(unsigned idx=0; idx < g_uRecWavBufsCount; idx++){
        short * &buf1 = localWavBufs[idx].data;
        unsigned &len1 = localWavBufs[idx].len;
        assert(len1 >= g_uWavBufLenMin);
        //if(len1 > remLen){
            len1 = remLen;
            buf1 = (short*)realloc(buf1, len1 * sizeof(short));
        //}
    }
}

static void prepare_rec_bufs(const vector<DataBlock> &datavec,
        RecogThreadSpace *regSpace)
{
    unsigned totalLen = 0;
    for(unsigned idx = 0; idx < datavec.size(); idx++){
        totalLen += datavec[idx].len;
    }
    assert(totalLen % sizeof(short) == 0);
    totalLen /= sizeof(short);
    release_rec_bufs(totalLen);
    WavBuf* localWavBufs = getRecWavBufs();
    regSpace->projData.m_pData = (char*)localWavBufs[0].data;
    regSpace->projData.m_iDataLen = totalLen * sizeof(short);
    regSpace->mcutBuf = NULL;
    regSpace->mcutBufLen = 0;
    regSpace->vadBuf = NULL;
    regSpace->vadBufLen = 0;
    if(g_uRecWavBufsCount > 1){
        regSpace->mcutBuf = localWavBufs[1].data;
        regSpace->mcutBufLen = totalLen;
    }
    if(g_uRecWavBufsCount > 2){
        regSpace->vadBuf = localWavBufs[2].data;
        regSpace->vadBufLen = totalLen;
    }
    
    unsigned curLen = 0;
    for(unsigned idx=0; idx < datavec.size(); idx++){
        memcpy(regSpace->projData.m_pData + curLen, datavec[idx].getPtr(), datavec[idx].len);
        curLen += datavec[idx].len;
    }
}

void* IoaRegThread(void *param)
{
    RecogThreadSpace &This_Buf = *(RecogThreadSpace*)param;
    char* &projData = This_Buf.projData.m_pData;
    unsigned long &curPid = This_Buf.projData.m_iPCBID;
    unsigned &projLen = This_Buf.projData.m_iDataLen;
	
	struct timeval cur_time;
	LOG_INFO(g_logger, "RegThread "<< This_Buf.threadIdx<< " of stream 0 has started ...");

    int hTLI;
    int hVAD = -1;
    if(g_bUseLid){
        hTLI = openTLI_dup();
    }
    if(g_bUseVAD){
        hVAD = openOneVAD(szLIDVADCfg);
        assert(hVAD != -1);
    }
    ProjectBuffer *ptrBuf = NULL;
    curPid = 0;
	while (true)
    {
        clockoutput_start("RecThread %d CLOCK_RECORD of one loop ", This_Buf.threadIdx);

        ptrBuf = obtainFullBufferTimeout(-1u);
        gettimeofday(&cur_time, NULL);
        if (ptrBuf != NULL)
        {
            ptrBuf->startMainReg();
            vector<DataBlock> datavec;
            assert(datavec.size() == 0);
            ptrBuf->getData(datavec);
            prepare_rec_bufs(datavec, &This_Buf);

            curPid = ptrBuf->ID;
            //这两个值为0，代表没有结果.
            This_Buf.result.m_iTargetID = 0;
            This_Buf.result.m_iAlarmType = 0;
            This_Buf.curTime = cur_time;
            //for TLI
            if(g_bUseLid)
            {
                lidRegProcess(This_Buf, hTLI);
            }

            if(g_bUseMusicDetect){
                musicProcess(This_Buf);
            }
            if(hVAD != -1){
                vadProcess(This_Buf, hVAD);
            }

            if(g_bUseSpk){
                spkRegProcess(This_Buf);   
            }
            /*
               if(g_bUseBamp && ptrBuf->getBampHit()){
               char savedfile[MAX_PATH];
               gen_spk_save_file(savedfile, m_TSI_SaveTopDir, NULL, ptrBuf->getPrjTime().tv_sec, ptrBuf->ID, NULL, NULL, NULL);
               if(!saveWave(This_Buf.projData.m_pData, This_Buf.projData.m_iDataLen, savedfile)){
               LOGFMT_WARN(g_logger, "in IoaRegThread failed to write wave to file %s.", savedfile);
               }
               else{
               LOGFMT_TRACE(g_logger, "PID=%lu SavedPath=%s have saved project pointed by bamp match.", ptrBuf->ID, savedfile);
               }
               }
               */
            This_Buf.addAccnt();
            //保存节目号，用于后面的去重.
            if(g_bSaveAfterRec){
                pthread_mutex_lock(&g_lockNewReported);
                NewReportedID.insert(map<unsigned long,ProjRecord_t>::value_type(curPid, ProjRecord_t("", cur_time.tv_sec)));
                pthread_mutex_unlock(&g_lockNewReported);
            }
            //保存节目数据. --- for debug.
            pthread_mutex_lock(&g_AllProjsDirLock);
            size_t tmpLen = strlen(g_szAllPrjsDir);
            if(tmpLen > 0 && if_directory_exists(g_szAllPrjsDir)){
                char savedfile[MAX_PATH];
                gen_spk_save_file(savedfile, g_szAllPrjsDir, NULL, cur_time.tv_sec, curPid, NULL, NULL, NULL);
                pthread_mutex_unlock(&g_AllProjsDirLock);
                if(!saveWave(projData, projLen, savedfile)){
                }
            }
            else{
                pthread_mutex_unlock(&g_AllProjsDirLock);
            }

            release_rec_bufs();
            ptrBuf->finishMainReg();
            returnBuffer(ptrBuf);
            LOGFMTD("PID=%lu %s.", curPid, clockoutput_end().c_str());
        }
        else{
            LOGFMTD("Project is NULL %s.", clockoutput_end().c_str());
        }
    }

    if(g_bUseLid) closeTLI_dup(hTLI);
    free(param);
    return NULL;
}


/////////////////////offline tasks////////////////////
//TODO 程序框架要升级，保证离线与在线互不影响.
/**
 * offline tasks related variable.
 */
struct OfflineLIDResult{
    OfflineLIDResult(int id=0, int lidCfg=0, int lidScore=0, int spkCfg=0, int spkScore=0)
        :id(id), lidCfg(lidCfg), lidScore(lidScore), spkCfg(spkCfg), spkScore(spkScore)
    { }
    int id;
    int lidCfg;
    int lidScore;
    int spkCfg;
    int spkScore;
};

vector<OfflineLIDResult> g_offResults;
unsigned int g_offTotalCount = 0;
const unsigned g_offCapacity = 100;
pthread_mutex_t g_offResLock = PTHREAD_MUTEX_INITIALIZER;

unsigned  sendOfflineData(const vector<string> & filelist)
{
    for(int i=0; i< filelist.size() && i < g_offCapacity; i++){
        const unsigned ONEPACKETLEN = 45 * 16000;
        FILE *fp = fopen(filelist[i].c_str(), "rb");
        if(fp == NULL){
            LOG_WARN(g_logger, "offline process, fail to open file "<< filelist[i]);
            return i;
        }
        char dataBuf[ONEPACKETLEN];
        fseek(fp, 44, SEEK_SET);
        while(true){
            size_t rnum = fread(dataBuf, 1, ONEPACKETLEN, fp);
            ProjectSegment pseg;
            pseg.pid = i;
            pseg.data = dataBuf;
            pseg.len = rnum;
            recvProjSegment(pseg, true);
            if(rnum < ONEPACKETLEN) break;
        }
        notifyProjFinish(i);
        fclose(fp);
    }
    return filelist.size();
}
static void offlineProcess()
{
    const char *taskFile = "ioacas/tasklist";
    vector<string> filelist = loadFileList(taskFile);
    if(filelist.size() == 0) return;
    int retrm = remove(taskFile);
    if(retrm == -1){
        LOG_WARN(g_logger, "offline process, fail to remove offline task file.");
    }
    g_offTotalCount = filelist.size();
    if(sendOfflineData(filelist) == 0){
        return;
    }
    pthread_mutex_lock(&g_offResLock);
    g_offResults.clear();
    pthread_mutex_unlock(&g_offResLock);
    const char *resultFile = "ioacas/results.txt";
    FILE *fp = fopen(resultFile, "w");
    if(fp == NULL){
        LOG_WARN(g_logger, "offline process, fail to open file to write results. "<< resultFile);
        return;
    }
    unsigned curNum = 0;
    while(true){
        pthread_mutex_lock(&g_offResLock);
        unsigned nextNum = g_offResults.size();
        if(nextNum > curNum){
            for(int idx=curNum; idx < nextNum; idx ++){
                fprintf(fp, "%d    %d    %d    %d    %d\n", g_offResults[idx].id, g_offResults[idx].lidCfg, g_offResults[idx].lidScore, g_offResults[idx].spkCfg, g_offResults[idx].spkScore);
            }
            curNum = nextNum;
        }
        pthread_mutex_unlock(&g_offResLock);
        if(curNum == g_offTotalCount){
            break;
        }
		struct timeval tv;
		tv.tv_sec = 3;
		tv.tv_usec = 0;
        select(1, NULL, NULL, NULL, &tv);

    }
    fclose(fp);

    pthread_mutex_lock(&g_offResLock);
    g_offTotalCount = 0;
    g_offResults.clear();
    pthread_mutex_unlock(&g_offResLock);
}

/*****************************
 * 离线识别过程，监听任务文件tasklist, 生成结果文件results.txt.
 *
 */
void *offLIDRegThread(void *param)
{
    while(true){
        offlineProcess();
		//wait 1 minutes, for saving establishment of TCP connection
		struct timeval timeout;
		timeout.tv_sec = 60;
		timeout.tv_usec = 0;
		select(0, NULL, NULL, NULL, &timeout);

    }
}

