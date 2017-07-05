/*************************************************************************
    > File Name: interface242.cpp
    > Author: zhengshurui
    > Mail:  zhengshurui@thinkit.cn
    > Created Time: Tue 06 Sep 2016 03:31:17 AM PDT
 ************************************************************************/

//#include "hhj.h"
#include "commonFunc.h"
#include "wav/waveinfo.h"
#include "utilites.h"
#include "ProjectBuffer.h"
#include "ioareg.h"

//TODO 这两个标记变量需要加锁.
static bool g_bInitialized = false;
static unsigned int g_iModuleID;

static ReceiveResult g_ReportResultAddr;

//////////////////---lid---
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
unsigned dft_getTypeFromId(unsigned int id)
{
    if(id == g_uLangWeirID) return g_uLangWeirType;
    else if(id == g_uLangTurkID) return g_uLangTurkType;
    else if(id == g_uLangAlabID) return g_uLangAlabType;
    else if(id == g_uVADID) return g_uVADType;
    else if(id == g_uMusicID) return g_uMusicType;
    else return g_uLangWeirType;
}

//control storing result.
std::vector<std::pair<unsigned int, std::pair<unsigned int, unsigned int> > > g_mLangReports;
//control reporting result.
static std::map<std::pair<unsigned int, unsigned int>, int> g_mLangReportFilter;

//------------------------spk part--------------
const unsigned short g_uSpkServType = 0x92;
static pthread_mutex_t g_SpkRepLock = PTHREAD_MUTEX_INITIALIZER;
static float g_fSpkReportThreshold = 0.0;
static map<unsigned, float> g_mSpkRepThds;
static const char* g_SpkRepThdsFile = "ioacas/SpeakerReport.txt";
static const char *g_SpkRepNofityFile = "ioacas/SpeakerReport";
static float getSpkRepThd(unsigned spkId)
{
    pthread_mutex_lock(&g_SpkRepLock);
    int ret = g_fSpkReportThreshold;
    if(g_mSpkRepThds.find(spkId) != g_mSpkRepThds.end()){
        if(ret < g_mSpkRepThds[spkId]){
            ret = g_mSpkRepThds[spkId];
        }
    }
    pthread_mutex_unlock(&g_SpkRepLock);
    return ret;
}

static void setSpkRepThd(float score, unsigned *cfgId = NULL)
{
    pthread_mutex_lock(&g_SpkRepLock);
    if(cfgId != NULL){
        g_mSpkRepThds[*cfgId] = score;
    }
    else{
        g_fSpkReportThreshold = score;
    }
    pthread_mutex_unlock(&g_SpkRepLock);
}
/*****************
 * param str eg: 11->1;10->2;
 * return list eg:[(11,1), (10,2)]
 */
static std::vector<std::pair<unsigned int, std::pair<unsigned int, unsigned int> > > parseLangReportsFromStr(const char*strLine)
{
	char tmpLine[256];
    std::vector<std::pair<unsigned int, std::pair<unsigned int, unsigned int> > > retm;
	strncpy(tmpLine, strLine, 256);
	char *st = tmpLine;
	while(true){
		char *tkEnd = strchr(st, ',');
		if(tkEnd != NULL) {
			*tkEnd = '\0';
		}
		unsigned int t, f, s;
        bool bparsed = false;
        if(strchr(st, ':') == NULL){
            if(sscanf(st, "%u %x", &f, &s) == 2){
                t = dft_getTypeFromId(s);
                bparsed = true;
            }
        }
        else{
            if(sscanf(st, "%u %x:%x", &f, &t, &s) == 3){
                bparsed = true;
            }
        }
        if(bparsed) retm.push_back(make_pair(f, std::make_pair(t, s)));
		if(tkEnd == NULL) break;
		st = tkEnd + 1;
	}
	return retm;
}

/**
 * reverse process of the func above.
 */
static std::string formLangReportsStr(std::vector<std::pair<unsigned int, std::pair<unsigned int, unsigned int> > >& langReports)
{
	ostringstream oss;
	for(unsigned i=0; i< langReports.size(); i++){
		oss<< (int)langReports[i].first<< " " << std::hex<< std::showbase<< (int)langReports[i].second.first<< ":" << (int)langReports[i].second.second << std::dec<< std::noshowbase << ",";
	}
	return oss.str();
}

static bool parseReportFilter(const char *strLine, std::map<std::pair<unsigned int, unsigned int>, int> &filter)
{
    char tmpLine[MAX_PATH];
    strncpy(tmpLine, strLine, MAX_PATH);
    char *st = tmpLine;
    unsigned char tmpCh;
    while(true){
        char *ed = strchr(st, ',');
        if(ed != NULL) *ed = '\0';
        unsigned int t, f;
        int s;
        bool bparsed = false;
        if(strchr(st, ':') == NULL){
            if(sscanf(st, "%x %d", &f, &s) == 2){
                t = dft_getTypeFromId(f);
                bparsed = true;
            }
        }
        else{
            if(sscanf(st, "%x:%x %d", &t, &f, &s) == 3){
                bparsed = true;
            }
        }
        if(bparsed) filter[make_pair(t, f)] = s;
        if(ed == NULL) break;
        st = ed + 1;
    }
    return true;
}
static std::string formReportFilterStr(std::map<std::pair<unsigned int, unsigned int>, int> &filter)
{
    std::ostringstream oss;
    for(std::map<std::pair<unsigned int, unsigned int>, int>::const_iterator it=filter.begin(); it != filter.end(); it++){
        oss<< std::hex<< std::showbase<< it->first.first<< ":"<< it->first.second << " "<< std::dec<< std::noshowbase<< it->second <<",";
    }
    return oss.str();
}

/**
 * 每行有两个数值， spkid spkscore
 *
 */
static void updateSpeakerThresholds()
{
    FILE *fp = fopen(g_SpkRepThdsFile, "r");   
    if(fp == NULL ){
        LOGFMT_WARN(g_logger, "updateSpeakerThresholds failed to open spkRepThdsFile %s", g_SpkRepThdsFile);
        return;
    }
    LOGFMT_INFO(g_logger, "updateSpeakerThresholds begin loading file %s.", g_SpkRepThdsFile);
    pthread_mutex_lock(&g_SpkRepLock);
    //g_mSpkRepThds.clear();
    char tmpline[100];
    while(fgets(tmpline, 100, fp) != NULL){
        unsigned cfgId;
        int score;
        if(sscanf(tmpline, "%u %d", &cfgId, &score) < 2) continue;
		if(g_mSpkRepThds.find(cfgId) == g_mSpkRepThds.end()) g_mSpkRepThds[cfgId] = g_fSpkReportThreshold;
		int curscore = g_mSpkRepThds[cfgId];
        if(curscore != score){
			LOGFMT_INFO(g_logger, "updateSpeakerThresholds update %u from %d to %d", cfgId, curscore, score);
		    g_mSpkRepThds[cfgId] = score;
		}
    }
    pthread_mutex_unlock(&g_SpkRepLock);
    fclose(fp);
}

const char szIoacasDir[]="./ioacas/";
ConfigRoom g_AutoCfg(((string)szIoacasDir + "SysFunc.cfg").c_str());

static char m_TSI_SaveTopDir[MAX_PATH]= "/home/ioacas/back_wave/";// save hited project.
static bool g_bDiscardable=true;// when feeding data.
bool g_bSaveAfterRec=false; // when after processing, for project ID.

char g_szEth4ReportIP[50];
static string g_strIp;
LoggerId g_logger;

std::map<unsigned long,ProjRecord_t> NewReportedID;
pthread_mutex_t g_lockNewReported = PTHREAD_MUTEX_INITIALIZER;


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
TransScore getScoreFunc(ScoreConfig *param = NULL)
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

static void initGlobal(BufferConfig &myBufCfg)
{
    string langReports = "14 0x20,";
    string langReportFilter = "0x20 99,";

    ScoreConfig ssCfg;
    ssCfg.m_0Value = 0;
    ssCfg.m_100Value = 100;
    Config_getValue(&g_AutoCfg, "", "eth4ReportIP", g_szEth4ReportIP);
    Config_getValue(&g_AutoCfg, "", "ifSkipSameProject", g_bSaveAfterRec);
    Config_getValue(&g_AutoCfg, "", "savePCMTopDir", m_TSI_SaveTopDir);
    Config_getValue(&g_AutoCfg, "lid", "languageReports", langReports);
    Config_getValue(&g_AutoCfg, "lid", "langReportFilter", langReportFilter);
    Config_getValue(&g_AutoCfg, "spk", "reportThreshold", g_fSpkReportThreshold);
    Config_getValue(&g_AutoCfg, "spk", "zeroScore", ssCfg.m_0Value);
    Config_getValue(&g_AutoCfg, "spk", "hundredScore", ssCfg.m_100Value);

    Config_getValue(&g_AutoCfg, "projectBuffer", "ifDiscardable", g_bDiscardable);
    Config_getValue(&g_AutoCfg, "projectBuffer", "waitSecondsStep", myBufCfg.waitSecondsStep);
    Config_getValue(&g_AutoCfg, "projectBuffer", "waitSeconds", myBufCfg.waitSeconds);
    Config_getValue(&g_AutoCfg, "projectBuffer", "waitLength", myBufCfg.waitLength);
    Config_getValue(&g_AutoCfg, "projectBuffer", "bufferBlockSize", myBufCfg.m_uBlockSize);
    Config_getValue(&g_AutoCfg, "projectBuffer", "bufferBlocksMin", myBufCfg.m_uBlocksMin);
    Config_getValue(&g_AutoCfg, "projectBuffer", "bufferBlocksMax", myBufCfg.m_uBlocksMax);

    myBufCfg.waitLength *= 16000;
	
    if(strlen(g_szEth4ReportIP) > 0) g_strIp = GetLocalIPByIF(g_szEth4ReportIP);
	g_mLangReports = parseLangReportsFromStr(langReports.c_str());
    parseReportFilter(langReportFilter.c_str(), g_mLangReportFilter);
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
            LOG4Z_VAR(g_szEth4ReportIP)
            LOG4Z_VAR(g_strIp)
            LOG4Z_VAR(g_AutoCfg.configFile)
            LOG4Z_VAR(m_TSI_SaveTopDir)
            LOG4Z_VAR(g_bDiscardable)
            LOG4Z_VAR(g_bSaveAfterRec)
            LOG4Z_VAR(g_fSpkReportThreshold)
             LOG4Z_VAR(ssCfg.m_0Value)
             LOG4Z_VAR(ssCfg.m_100Value)
            LOG4Z_VAR(formLangReportsStr(g_mLangReports).c_str()) LOG4Z_VAR(formReportFilterStr(g_mLangReportFilter).c_str())
            LOG4Z_VAR(g_AutoCfg.configFile)
            LOG4Z_VAR(myBufCfg.waitSecondsStep)
            LOG4Z_VAR(myBufCfg.waitSeconds)
            LOG4Z_VAR(myBufCfg.waitLength )
            LOG4Z_VAR(myBufCfg.m_uBlockSize)
            LOG4Z_VAR(myBufCfg.m_uBlocksMin)
            LOG4Z_VAR(myBufCfg.m_uBlocksMax)
            );
    updateSpeakerThresholds();
    getScoreFunc(&ssCfg);
}


static void *ioacas_maintain_procedure(void *);
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
    initGlobal(buffconfig);

    init_bufferglobal(buffconfig);

    if(!ioareg_init()){
        return 1;
    }
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

	LOG_INFO(g_logger, "======================ioacas module initialize success=========================");
    g_bInitialized = true;
    return 0;
}

int SendData2DLL(WavDataUnit *p)
{
    assert(p->m_iDataLen % 2 == 0);
    const unsigned HEADSIZE = 100;
    char szHead[HEADSIZE];
    snprintf(szHead, HEADSIZE, "SendData2DLL PID=%lu SIZE=%u ", p->m_iPCBID, p->m_iDataLen);
    if(!g_bInitialized){
        LOG_WARN(g_logger, szHead<<"fail to receive data as ioacas being uninitialized.");
        return -1;
    }
    clockoutput_start("SendData2DLL");
    if(g_bSaveAfterRec){
        pthread_mutex_lock(&g_lockNewReported);
        if(NewReportedID.find(p->m_iPCBID) != NewReportedID.end())
        {
            pthread_mutex_unlock(&g_lockNewReported);
            LOG_WARN(g_logger, szHead<< "abandoned for being processed recently.");
            return -1;
        }
        pthread_mutex_unlock(&g_lockNewReported);
    }

    ProjectSegment pseg;
    pseg.pid = p->m_iPCBID;
    pseg.data = p->m_pData;
    pseg.len = p->m_iDataLen;
    recvProjSegment(pseg, !g_bDiscardable);
    string output = clockoutput_end();
    LOGFMTT(output.c_str());
    LOG_TRACE(g_logger, szHead<< "have put data to GlobalBuffer.");
    return 0;
}

int CloseDLL()
{
    rlse_bufferglobal();
    ioareg_rlse();
    return 0;
}

int AddCfg(unsigned int id, 
        const char *strName,
        const char *strConfigFile,
        int iType,
        int iHarmLevel)
{
    if(iType == g_uSpkServType && g_bUseSpk){
        FILE* fp = fopen(strConfigFile, "rb");
        if(fp == NULL) {
            LOGFMT_ERROR(g_logger, "AddCfg failed to open file: %s", strConfigFile);
             return -30;   
        }
        fseek(fp, 0, SEEK_END);
        unsigned uLen = ftell(fp);
        fseek(fp, 0, SEEK_SET);
        char *pData = new char[uLen];
        if(pData == NULL){
            LOGFMT_ERROR(g_logger, "in AddCfg, failed to allocate memory, size: %u", uLen);
            fclose(fp);
            return -40;
        }
        unsigned rlen = fread(pData, 1, uLen, fp);
        fclose(fp);
        return AddCfgByBuf(pData, uLen, id, iType, iHarmLevel);
    }
    return 0;
}

int AddCfgByBuf(const char *pData,
        int iDataBytes,
        unsigned int id,
        int iType,
        int iHarmLevel)
{
    if(iType == g_uSpkServType && g_bUseSpk){
        SpkInfoChd *curspk = new SpkInfoChd(id, iType, iHarmLevel);
        if(!spkex_addSpk(curspk, const_cast<char*>(pData), iDataBytes)){
            delete curspk;
            return 0;
        }
		LOGFMT_INFO(g_logger, "add one speaker success, id: %u", id);

        return 1;
    }
    return 0;
}

int AddCfgByDir(int iType, const char *strDir)
{
    if(iType == g_uSpkServType && g_bUseSpk){
        int iRet = procFilesInDir(strDir, addSpkPerFile);
        LOGFMT_INFO(g_logger, "AddCfgByDir finishing load spks in dir: %s, SPKCount=%d.", strDir, iRet);
        return iRet;
    }
    return 0;
}
int RemoveAllCfg(int iType)
{
    if(iType == g_uSpkServType && g_bUseSpk){
        vector<unsigned long> allids;
        spkex_getAllSpks(allids);
        for(unsigned idx=0; idx < allids.size(); idx++){
            spkex_rmSpk(allids[idx]);
        }
        return 1;
    }
    return 0;
}
int RemoveCfgByID(unsigned int id, int iType, int iHarmLevel)
{
    if(iType == g_uSpkServType && g_bUseSpk){
        spkex_rmSpk(id);
        return 1;
    }
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
        result.m_iTargetID |= 0x200;
        brep = true;
    }
    else if(result.m_iAlarmType == g_uSpkServType){
        result.m_fLikely = getScoreFunc()(result.m_fLikely);
        if(result.m_fLikely >= getSpkRepThd(configID)) brep = true;
    }
    
    if(brep){
        g_ReportResultAddr(g_iModuleID, &result);
        if(writeLog != NULL){
            len += sprintf(writeLog + len, "Reported ");
        }
        const char* debugDir = "ioacas/debug/";
        if(if_directory_exists(debugDir)){
            char wholePath[MAX_PATH];
            sprintf(wholePath, "%sMessage_%lu", debugDir, pid);
            save_binary_data(wholePath, &result, sizeof(CDLLResult), result.m_pDataUnit[0], sizeof(WavDataUnit), NULL);
        }
    }
    return true;
}

/**
 * one maintain operation for clearup routine.
 * clear up records before <seconds> seconds go.
 * this function  should be confined in lock of g_lockNewReported.
 */
void maintain_newreported(time_t curtime, unsigned seconds)
{
	debugstring_newreported("before maintance");
	vector<unsigned long> delID;
	for(map<unsigned long,ProjRecord_t>::iterator IDs = NewReportedID.begin();IDs !=NewReportedID.end();++IDs)
	{
		if(curtime - (*IDs).second.timemark >= seconds)
			delID.push_back((*IDs).first);
	}
	for(unsigned int tt = 0;tt < delID.size();++tt)
		NewReportedID.erase(delID[tt]);
	debugstring_newreported("after maintance");
}

void *ioacas_maintain_procedure(void *)
{
    while(true){
        sleep(3);
        //select(NULL, NULL, NULL, NULL, &steptv);
        time_t cur_time;
        time(&cur_time);

        if(true){
            //maitain project filter.
            pthread_mutex_lock(&g_lockNewReported);
            static time_t projectfilterlasttime = 0;
            if(cur_time   > 3600 + projectfilterlasttime){
                projectfilterlasttime = cur_time;
                maintain_newreported(cur_time, 3600 * 5);
            }
            pthread_mutex_unlock(&g_lockNewReported);
        }
        if(true){
            //update some configurations.
            static time_t lasttime;
            if(cur_time > 3 + lasttime){
                lasttime = cur_time;
                if(g_AutoCfg.checkAndLoad()){
                    if(g_AutoCfg.isUpdated("spk", "reportThreshold")){
                        float score;
                        Config_getValue(&g_AutoCfg, "spk", "reportThreshold", score);
                        setSpkRepThd(score);
                        LOGFMT_INFO(g_logger, "updateConfig spk.reportThreshold updated to %.2f", score);
                    }
                }
            }
        }
        if(true){
            FILE *fp = fopen(g_SpkRepNofityFile, "r");
            if(fp != NULL){
                updateSpeakerThresholds();
                fclose(fp);
                remove(g_SpkRepNofityFile);
            }
        }
        ioareg_maintain_procedure(cur_time);
    }
    return NULL;
}
