/*************************************************************************
    > File Name: test242.cpp
    > Author: zhengshurui
    > Mail:  zhengshurui@thinkit.cn
    > Created Time: Thu 08 Sep 2016 11:23:55 PM PDT
 ************************************************************************/

#include <dirent.h>
#include <time.h>
#include <sys/time.h>
#include <sys/select.h>
#include <sys/types.h>
#include <regex.h>
#include <errno.h>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <signal.h>
#include <dlfcn.h>

#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <set>
#include <map>
#include <memory>
using namespace std;

#include "apue.h"
#include "myiofuncs.h"
#include "comm_struct.h"
#include "waveinfo.h"
//#include "myqueue.h"
#include "../../include/interface242.h"
//#include "../../src/log4z.h"
//#include "../../src/commonFunc.h"
//extern zsummer::log4z::ILog4zManager *g_Log4zManager;

typedef int (*FuncInitDll)(int iPriority,
        int iThreadNum,
        int *pThreadCPUID,
        ReceiveResult func,
        int iDataUnitBufSize,
        char *path,
        unsigned int iModuleID);
typedef int (*FuncSendData2DLL)(WavDataUnit *p);
typedef int (*FuncCloseDll)();
typedef int (*FuncAddCfgByDir)(int iType, const char *strDir);
typedef int (*FuncAddCfg)(int, const char*, const char*, int, int);
typedef int (*FuncRemoveAllCfg)(int iType);
typedef int (*FuncNotifyProjFinish)(unsigned long);
typedef bool (*FuncIsAllFinished)();
void *g_Handler4Ioacas;
FuncInitDll funcInitDll;
FuncSendData2DLL funcSendData2Dll;
FuncCloseDll funcCloseDll;
FuncAddCfgByDir funcAddCfgByDir;
FuncAddCfg funcAddCfg;
FuncRemoveAllCfg funcRemoveAllCfg;
FuncNotifyProjFinish funcNotifyProjFinish;
FuncIsAllFinished funcIsAllFinished;
#define MAX_PATH 512
#define QUATEMACRO(x) #x
#define QUATEMACROS(x) QUATEMACRO(x)
unsigned getPackBytesByCompl(){
    unsigned ret = 720000;
#ifdef TESTDIR
    if(strncmp(QUATEMACROS(TESTDIR), "testZP", 9) == 0){
        ret = 48000;
    }
#endif
    return ret;
}

unsigned g_packetBytes= getPackBytesByCompl();
bool g_bSingleProjectMode = false;
//namespace zen4audio{
//    void notifyProjFinish(unsigned long);
//    bool isAllFinished();
//}

struct ProjInfo{
public: 
	ProjInfo(): id(0), dataLen(0), sendLen(0), retLen(0), iAlarmType(0), starttime(0), recatchtime(0), targetID(0), iLikely(0)
	{
        filePath[0] = '\0';
	}
	~ProjInfo(){
	}
    ProjInfo(const ProjInfo& other){
        memcpy(this, &other, sizeof(ProjInfo));
    }
    ProjInfo& operator=(const ProjInfo& other){
        if(this != &other){
            memcpy(this, &other, sizeof(ProjInfo));
        }
        return *this;
    }
	string toString() const{
		ostringstream oss;
        const char * basename = strrchr(filePath, '/');
        if(basename == NULL){
            basename = filePath;
        }
        else{
            basename ++;
        }

		oss<< "id: "<<id<<" file: "<< basename;
        if(starttime == 0){
            return oss.str();
        }
        oss<< " alarmType: "<< iAlarmType<<" dataLen: "<< dataLen<<" sendLen: " << sendLen;
        if(recatchtime == 0){
            return oss.str();
        }
        oss<< " retLen: "<< retLen<< " elapseTime: "<< recatchtime - starttime<< " targetID: "<< targetID<< " likely: "<< iLikely;
		return oss.str();
	}
	unsigned long id; //unique identity of one object.
    //std::string filePath;
    char filePath[MAX_PATH];
	long int dataLen; //total size anticipated by some way.
	long int sendLen;//already send num of data 
    long int retLen; 
	time_t starttime;
    unsigned int iAlarmType;
	time_t recatchtime;
    unsigned int targetID;
    int iLikely;
};

map<unsigned long long, ProjInfo> g_mId2Infos;
pthread_mutex_t g_ProjIdsLock = PTHREAD_MUTEX_INITIALIZER;
FILE *g_fpRes = NULL;
float g_fWaitSecs = 0.01;

int receive_result(unsigned int iModuleID, CDLLResult *res)
{
    pthread_mutex_lock(&g_ProjIdsLock);
    map<unsigned long long, ProjInfo>::iterator it = g_mId2Infos.find(res->m_pDataUnit[0]->m_iPCBID);
    if(it == g_mId2Infos.end()){
        pthread_mutex_unlock(&g_ProjIdsLock);
        fprintf(stderr, "WARN miss project information for retrival result of ioacas.\n");
        return 0;
    }
    ProjInfo curobj = it->second;
    g_mId2Infos.erase(it);
    pthread_mutex_unlock(&g_ProjIdsLock);

	if(curobj.recatchtime == 0){
		curobj.recatchtime = time(NULL);
        curobj.retLen = res->m_pDataUnit[0]->m_iDataLen;
        curobj.targetID = res->m_iTargetID;
        curobj.iLikely = (int)res->m_fLikely;
	}
    string tmpStr = curobj.toString() + "\n";
    if(g_fpRes != NULL){
        fwrite(tmpStr.c_str(), 1, tmpStr.size(), g_fpRes);
    }
    //LOGI(tmpStr);
    printf(tmpStr.c_str());
    
    return 0;
}

/**
 * 从模式*_<pid>_*中提取出节目名，当节目名唯一时，提取成功，否则，提取失败。
 */
void parseWavePid(string wavePath, unsigned long& pid, unsigned long defPid)
{
    pid = defPid;
    while(true){
		size_t fipl = wavePath.find_first_of("_");
		if(fipl == string::npos) break;
		size_t sepl = wavePath.find_first_of("_", fipl+1);
		if(sepl == string::npos) break;
		string strpid = wavePath.substr(fipl + 1, sepl - fipl -1);
		unsigned long long ullvar;
		if(sscanf(strpid.c_str(), "%llu", &ullvar) != 1) break;
        pid = ullvar;
        break;
    }
}
struct ProjInfoIter{
    string audioRoot;
    DIR *pDir;
    static unsigned long projectCount;
    static unsigned long otherCount;

    ProjInfoIter():
        pDir(NULL)
    {}
    ProjInfoIter(const char *rt):
        pDir(NULL)
    {
        open(rt);
    }
    ~ProjInfoIter(){
        if(pDir != NULL){
            closedir(pDir);
            pDir = NULL;
        }
    }
    void open(const char *rt);
    ProjInfo* next();
};
unsigned long ProjInfoIter::projectCount = 0;
unsigned long ProjInfoIter::otherCount = 0;
void ProjInfoIter::open(const char *rt)
{
    if(pDir != NULL){
        closedir(pDir);
    }
    if((pDir = opendir(rt)) == NULL){
        err_sys("cannot open dir %s.", rt);
    }
    audioRoot = rt;
}
ProjInfo* ProjInfoIter::next()
{
    if(pDir == NULL) return NULL;
    char *curname = NULL;
    struct dirent *pDirent;
    while((pDirent = readdir(pDir)) != NULL){
        curname = pDirent->d_name;
        if(curname[0] == '.') continue;
        char *lastdotptr = strrchr(curname, '.');
        if(lastdotptr != NULL && strcmp(lastdotptr, ".wav") == 0) break;
    }
    if(pDirent != NULL){
        ProjInfo* proj = new ProjInfo;
        snprintf(proj->filePath, MAX_PATH, "%s%s", audioRoot.c_str(), curname);
        parseWavePid(curname, proj->id, static_cast<unsigned long>(-1));
        if(proj->id == static_cast<unsigned long>(-1)){
            proj->id = otherCount;
            otherCount ++;
        }
        else{
            projectCount ++;
        }
        return proj;
    }
    return NULL;
}

string g_AudioDir;
ProjInfoIter g_iterProjInfo;
pthread_mutex_t sendCSLocker = PTHREAD_MUTEX_INITIALIZER;
/**
 *
 */
void *sendProjectProcess(void *param)
{
	pthread_t curpid = pthread_self();
    unsigned count = 0;
	char *databuf = (char*)malloc(g_packetBytes);
    while(true){
        pthread_mutex_lock(&sendCSLocker);
        ProjInfo *ptask = g_iterProjInfo.next();
        pthread_mutex_unlock(&sendCSLocker);
        if(ptask == NULL) break;
        printf("<%u> get task[%d]  %s\n", (unsigned)curpid, count++, ptask->filePath);
        ifstream ifs(ptask->filePath, ios::binary | ios::in);
        if(ifs.fail()){
			//LOGFMTE("fail to open file %s\n", ptask->filePath);
			fprintf(stderr, "fail to open file %s\n", ptask->filePath);
			continue;
        }
        PCM_HEADER wavHeader;
        long skipLen;
        if(!read_wave_header(ifs, &wavHeader, &skipLen)){
            fprintf(stderr, "fail to parse pcm head from file %s.\n", ptask->filePath);
            ifs.close();
            delete ptask;
            continue;
        }
	if(wavHeader.fmt.wFormatTag != 1){
		fprintf(stderr, "the wave is not pcm linear, skip it. file: %s.\n", ptask->filePath);
		ifs.close();
		delete ptask;
		continue;
	}
        if(ifs.eof()){
            fprintf(stderr, "no wave data in file %s.\n", ptask->filePath);
            ifs.close();
            delete ptask;
            continue;
        }
        long headlen = ifs.tellg();
        ifs.seekg(0, ios_base::end);
		ptask->dataLen = ifs.tellg() - headlen;
        ifs.seekg(headlen);
        ptask->sendLen = 0;
        /*
        pthread_mutex_lock(&g_ProjIdsLock);
        g_mId2Infos[ptask->id] = *ptask;
        pthread_mutex_unlock(&g_ProjIdsLock);
        */

		while(true)
		{
            ifs.read(databuf, g_packetBytes);
			int bulksize = ifs.gcount();
            if(bulksize % 2 != 0){
                fprintf(stderr, "the length of data is odd, skiplen: %d, headlen: %ld, datalen: %ld, file %s.\n", skipLen, headlen, ptask->dataLen, ptask->filePath);
            }
			if(g_packetBytes == 48000 && bulksize == g_packetBytes || g_packetBytes != 48000 && bulksize > 0){
                WavDataUnit dataUnit;
                dataUnit.m_iPCBID = ptask->id;
                dataUnit.m_iDataLen = bulksize;
                dataUnit.m_pData = databuf;
                if(funcSendData2Dll(&dataUnit) == 0){
                    pthread_mutex_lock(&g_ProjIdsLock);
                    if(g_mId2Infos.find(ptask->id) != g_mId2Infos.end()){
                        ProjInfo& curobj = g_mId2Infos[ptask->id];
                        curobj.sendLen += bulksize;
                    }
                    pthread_mutex_unlock(&g_ProjIdsLock);
                }
                struct timeval tv;
                tv.tv_sec = (int)g_fWaitSecs;
                tv.tv_usec = (g_fWaitSecs - tv.tv_sec) * 1000000;
                select(0, NULL, NULL, NULL, &tv);
			}
            else if(bulksize > 0){
                fprintf(stderr, "WARN the last packet is of the size less than %d, and is discarded.\n", g_packetBytes);
            }

			if(ifs.fail() && !ifs.eof()){
				//LOGFMTE("error: %s; file %s\n", strerror(errno), ptask->filePath);
				printf("error: %s; file %s\n", strerror(errno), ptask->filePath);
				break;
			} else if(ifs.eof()){
				break;
			}
		}
        ifs.close();
        
        bool bSendData = true;
        pthread_mutex_lock(&g_ProjIdsLock);
        if(g_mId2Infos.find(ptask->id) != g_mId2Infos.end()){
            ProjInfo& curobj = g_mId2Infos[ptask->id];
            if(curobj.sendLen != 0){
                bSendData = true;
                curobj.starttime = time(NULL);
            }
        }
        pthread_mutex_unlock(&g_ProjIdsLock);
        if(bSendData){
            funcNotifyProjFinish(ptask->id);
        }

        delete ptask;
        if(g_bSingleProjectMode){
            while(!funcIsAllFinished()){
                sleep(3);
            }
        } 
    }
    free(databuf);
    return NULL;
}
void send_project_parallel(int paranum)
{
    if(paranum < 1) return;
    g_iterProjInfo.open(g_AudioDir.c_str());
    pthread_mutex_lock(&g_ProjIdsLock);
    g_mId2Infos.clear();
    pthread_mutex_unlock(&g_ProjIdsLock);
    pthread_t *allthds = (pthread_t*)malloc(paranum * sizeof(pthread_t));
    for(int i=0; i<paranum; i++){
        int crtret = pthread_create(&(allthds[i]), NULL, sendProjectProcess, NULL);
        if(crtret != 0){
            free(allthds);
            err_sys("error while creating pthread");
        }
    }

    for(int i =0; i< paranum; i++){
        int err = pthread_join(allthds[i], NULL);
        if(err != 0){
            //LOGFMTI("fail to join thread: idx %d", i);
            printf("fail to join thread: idx %d", i);
        }
    }
    //g_iterProjInfo.~ProjInfoIter();
    free(allthds);
}

void fetch_all_options(int argc, char* argv[], const char *opts, map<char,  string> &optbox, const char *szHelp)
{
	opterr = 1;
	char c;
	while( (c = getopt(argc, argv, opts)) != EOF){
		switch (c){
			case '?':
				fprintf(stderr,"%s\n", szHelp);
				exit(1);
				break;
			default:
				if(optarg == NULL) optbox[c] = "";
				else
					optbox[c] = optarg;
		}
	}
}

void saveLeftItems()
{
    unsigned culcnt = 0;
    pthread_mutex_lock(&g_ProjIdsLock);
    for(map<unsigned long long, ProjInfo>::const_iterator it=g_mId2Infos.begin(); it!=g_mId2Infos.end(); it++){
        if(it->second.recatchtime == 0){
            culcnt++;
            string tmpStr = it->second.toString() + "\n";
            fwrite(tmpStr.c_str(), 1, tmpStr.size(), g_fpRes);
            fprintf(stderr, "saveLeftItems PID=%lu no result from Ioacas module.\n", it->second.id);
        }
    }
    g_mId2Infos.clear();
    pthread_mutex_unlock(&g_ProjIdsLock);
}
void interactWithMe(int secs=3)
{
    fprintf(stdout, "before starting this round, please strike key s to stop, or continue after %d secs:", secs);
    char tmpstr[256];
    char *retfgets = fgetstimeout(tmpstr, 256, secs);
    if(retfgets != NULL && tmpstr[0] == 's'){
        fprintf(stdout, "you have stopped me, press c to restart:");
        while(1){
            fgets(tmpstr, 256, stdin);
            if(tmpstr[0] == 'c' && tmpstr[1] == '\n'){
                break;
            }
            fprintf(stdout, "you have stopped me, press c to restart:");
        }
    }
    else{
        fprintf(stdout, "\n");
    }
}
void interhandler(int num)
{
    fprintf(stderr, "handling interruption envent.\n");
    exit(1);
}
#ifndef IOACAS
#define IOACAS ioacas
#endif
#define LIBPATH(x) #x"/libIOACAS.so"
#define LIBPATHM(x) LIBPATH(x)
void initIoacas()
{
    const char *libPath = LIBPATHM(IOACAS);
    void * hdl = dlopen(libPath, RTLD_LAZY );
    if(hdl == NULL){
        fprintf(stderr, "ERROR fail to load library %s, error: %s.\n", libPath, dlerror());
        exit(1);
    }
    
    dlerror();
    funcInitDll = (FuncInitDll)dlsym(hdl, "InitDLL");
    char *errStr = dlerror();
    if(errStr != NULL){
        fprintf(stderr, "ERROR %s.\n", errStr);
        exit(1);
    }
    dlerror();
    funcSendData2Dll = (FuncSendData2DLL)dlsym(hdl, "SendData2DLL");
    errStr = dlerror();
    if(errStr != NULL){
        fprintf(stderr, "ERROR %s.\n", errStr);
        exit(1);
    }
    dlerror();
    funcCloseDll = (FuncCloseDll)dlsym(hdl, "CloseDLL");
    errStr = dlerror();
    if(errStr != NULL){
        fprintf(stderr, "ERROR %s.\n", errStr);
        exit(1);
    }

    funcAddCfgByDir = (FuncAddCfgByDir)dlsym(hdl, "AddCfgByDir");
    errStr = dlerror();
    if(errStr != NULL){
        fprintf(stderr, "ERROR %s.\n", errStr);
        exit(1);
    }
    funcAddCfg = (FuncAddCfg)dlsym(hdl, "AddCfg");
    errStr = dlerror();
    if(errStr != NULL){
        fprintf(stderr, "ERROR %s.\n", errStr);
        exit(1);
    }
    funcRemoveAllCfg = (FuncRemoveAllCfg)dlsym(hdl, "RemoveAllCfg");
    errStr = dlerror();
    if(errStr != NULL){
        fprintf(stderr, "ERROR %s.\n", errStr);
        exit(1);
    }
    dlerror();
    funcNotifyProjFinish = (FuncNotifyProjFinish)dlsym(hdl, "notifyProjFinish");
    errStr = dlerror();
    if(errStr != NULL){
        fprintf(stderr, "ERROR %s.\n", errStr);
        exit(1);
    }
    dlerror();
    funcIsAllFinished = (FuncIsAllFinished)dlsym(hdl, "isAllFinished");
    errStr = dlerror();
    if(errStr != NULL){
        fprintf(stderr, "ERROR %s.\n", errStr);
        exit(1);
    }

    g_Handler4Ioacas = hdl;
}
void relsIoacas()
{
    dlclose(g_Handler4Ioacas);
}
int main(int argc, char* argv[])
{
    initIoacas();
    typedef void (*sighandler_t) (int);
    sighandler_t rets = signal(SIGINT, interhandler);
    if(rets == SIG_ERR){
        fprintf(stderr, "error catch sigkill, err: %s.\n", strerror(errno));
    }
	if(atexit(saveLeftItems) != 0){
		fprintf(stderr, "error: fail to register exiting function savesendlist.\n");
	}
    const char *szHelp = " usage: program -s -d <wavedir> -p <threads> -k <result file>";
    map<char, string> allopts;
    fetch_all_options(argc, argv, "bsd:p:k:", allopts, szHelp);
    int parallelNum = 1;
    bool g_bNonStop = false;
    char sendInfoFile[MAX_PATH];
    sprintf(sendInfoFile, "%s", "result.txt");
    g_AudioDir = "TestWave";
	if(allopts.find('d') != allopts.end()){
        g_AudioDir = allopts['d'];
	}
    size_t rtlen = g_AudioDir.size();
    if(g_AudioDir[rtlen - 1] != '/'){
        g_AudioDir.append("/");
    }
	if(allopts.find('p')!= allopts.end()){
		parallelNum = atoi(allopts['p'].c_str());
	}
	if(allopts.find('k') != allopts.end()){
		sprintf(sendInfoFile, "%s", allopts['k'].c_str());
	}
    if(allopts.find('s') != allopts.end()){
        g_bSingleProjectMode = true;
    }
    if(allopts.find('b') != allopts.end()){
        g_bNonStop = true;
    }
    //LOGI("-----<<<<<>>>>>-----\nWaveDir: "<< rootDir<< "\nSendingThreadNum: "<< parallelNum<< "\nresultfile: "<< sendInfoFile);
    ostringstream oss;
    oss<<"-----<<<<<>>>>>-----\nWaveDir: "<< g_AudioDir.c_str()<< "\nSendingThreadNum: "<< parallelNum<< "\nresultfile: "<< sendInfoFile<< endl;
    oss<< "singleProjectMode: "<< g_bSingleProjectMode<< endl;
    oss<< "nonStop: "<< g_bNonStop<< endl;
    printf(oss.str().c_str());

    g_fpRes = fopen(sendInfoFile, "w");
    if(g_fpRes == NULL){
        //LOGE("fail to open file to write result, file: "<< sendInfoFile);
        fprintf(stderr, "fail to open file to write result, file: %s.\n", sendInfoFile);
    }
    int iPriority = 0;
    int iThreadNum = 1;
    int *pThreadCPUID = NULL;
    int iDataUnitBufSize = 0;
    char workPath[MAX_PATH];
    workPath[0] = '\0';
    unsigned int iModuleID = 0;
    int retInit = funcInitDll(iPriority, iThreadNum, pThreadCPUID, receive_result, iDataUnitBufSize, workPath, iModuleID);
    if(retInit != 0){
        //LOGE("failed in InitDll");
        fprintf(stderr, "failed in InitDll");
        exit(1);
    }
    char mdlPath[MAX_PATH];
    int servType4Model = 146;
    strcpy(mdlPath, "SpkModel");
    funcAddCfgByDir(servType4Model, mdlPath);
    do{
        interactWithMe();
        send_project_parallel(parallelNum);
    }while(g_bNonStop);
    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 0;
    while(true){
        if(funcIsAllFinished()){
            cout<< "checked all tasks finishing in BufferGlobal.\n";
             break;   
        }
        tv.tv_sec = 5;
        select(0, NULL, NULL, NULL, &tv);
    }
	cout<< ">>>>>>>>>>>>>finish test<<<<<<<<<<<<<<<\n";
    interactWithMe(1000);
    funcRemoveAllCfg(servType4Model);
    funcCloseDll();
    relsIoacas();
	return 0;
}
