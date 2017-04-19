/*************************************************************************
    > File Name: myMain.cpp
    > Author: ma6174
    > Mail: ma6174@163.com 
    > Created Time: Tue 03 Feb 2015 03:13:05 AM PST
 ************************************************************************/

#include "../common/apue.h"
#include <dirent.h>
#include <time.h>
#include <sys/time.h>
#include <sys/select.h>
#include <sys/types.h>
#include <regex.h>
#include <errno.h>
#include <cstdlib>

#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <set>
#include <map>
#include <memory>
using namespace std;

#include "comm_struct.h"
#include "myqueue.h"
//#include <dlfcn.h>
#define MIN_VALID_WAVE_LEN 45 * 8000 *2

extern bool Ioacas_IsRecReady();
//short int g_testmode = 0;
int g_intervalSeconds = 0;
unsigned g_packetBytes = 320000;
//#define DLL_FILE_NAME "libinterface.so"
const char g_szLogFile[] = "./ioacas/ioacas_.log";
static ifstream g_ifsIoacasLog;

//typedef int (*InitFunc) (const unsigned long ,const short);
//typedef	void (*Func)(unsigned long long, const char*, unsigned int, const char*, unsigned int);
struct ProjInfo{
	private: static pthread_mutex_t idAccuLock;
	private: static int globalId;
public: 
	ProjInfo(string str =""): filePath(str), dataLen(0), sendLen(0), starttime(0), recatchtime(0)
	{
		pthread_mutex_lock(&idAccuLock);
		id = ++globalId;
		pthread_mutex_unlock(&idAccuLock);
		pthread_mutex_init(&m_objLock, NULL);
	}
	~ProjInfo(){
		pthread_mutex_destroy(&m_objLock);
	}
	string toString(){
		ostringstream os;
		os<< "id: "<<id<<" filePath: "<< filePath.c_str() <<" dataLen: "<< dataLen<<" sendLen: " << sendLen
			<<" elapseTime: "<< recatchtime - starttime;
		return os.str();
	}
	string filePath;//source audio file
	unsigned long long id; //unique identity of one object.
	long int dataLen; //total size anticipated by some way.
	long int sendLen;//already send num of data 
	time_t starttime;
	string postFilePath;//file path in result packet.
	time_t recatchtime;
	pthread_mutex_t m_objLock;
};
char g_sendInfoFile[512];
map<unsigned long long, ProjInfo*> g_mId2Infos;
pthread_mutex_t ProjInfo::idAccuLock = PTHREAD_MUTEX_INITIALIZER;
int ProjInfo::globalId = 0;

vector<string> get_file_list(const char *root)
{
	vector<string> ret;
	DIR * pDir;
    struct dirent *pDirent;
    if((pDir = opendir(root)) == NULL)
		err_sys("can't open %s", root);
	while((pDirent = readdir(pDir)) != NULL)
	{
		char *curname = pDirent->d_name;
        if(curname[0] == '.') continue;
		char *lastdotptr = strrchr(curname, '.');
		if(lastdotptr != NULL && strcmp(lastdotptr, ".wav")==0){
			ret.push_back(pDirent->d_name);
		}
	}
	return ret;
}

const char *debug_id2Infos(){
	static ostringstream os;
	os.str("");
	os << "size: "<<g_mId2Infos.size()<<endl;
	for(map<unsigned long long, ProjInfo*>::iterator it=g_mId2Infos.begin(); it!=g_mId2Infos.end(); it++){
		os<< it->first<<"-->"<<it->second->toString()<<endl;
	}
	return os.str().c_str();
}
static inline bool recordRecatchInfo(unsigned long long uPid, string strPath)
{
    bool ret =false;
    if(g_mId2Infos.find(uPid) != g_mId2Infos.end()){
        ProjInfo *curobjptr = g_mId2Infos[uPid];
        pthread_mutex_lock(&(curobjptr->m_objLock));
        if(curobjptr->recatchtime == 0){
            ret =true;
            curobjptr->recatchtime = time(NULL);
            curobjptr->postFilePath = strPath;
        }
        pthread_mutex_unlock(&(curobjptr->m_objLock));
    }
    return ret;
}
static string csubstr(const char *str, unsigned st, unsigned ed)
{
    const unsigned maxLineLength = 1024;
    char curStr[maxLineLength];
    unsigned len = ed- st;
    len = len > maxLineLength -1 ? maxLineLength - 1 : len;
    strncpy(curStr, str + st, len);
    curStr[len] = '\0';
    return string(curStr);
}
/**
 * monitor the new arrival of log in file ./ioacas/ioacas_.log, until get the idea all the projects are done.
 *
 */
void monitorRecognitionProgress()
{
    ifstream &ifs = g_ifsIoacasLog;
    struct timeval ts;
    ts.tv_sec = 2;
    ts.tv_usec = 0;

    const size_t maxLineLength = 1024;
    char errBuf[maxLineLength];
    const size_t maxSubexprNum = 10;
    regmatch_t subMatches[maxSubexprNum];
    const char patUseSpk[] = "g_bUseSpk=(\\w+)";
    const char patUseLid[] = "g_bUseLid=(\\w+)";
    const char patSpkReport[] = "SPKREG PID=([0-9]+) WavLen=";
    const char patLidReport[] = "SPKLID PID=([0-9]+) WavLen=";
    const char *patSavePath = "DATASAVEPATH=([^ ]+)";
    const char patTest[] = "SPKREG PID=";
    regex_t reUseLid, reUseSpk, reLidReport, reSpkReport, reSavePath;
    int errCode = regcomp(&reUseLid, patUseLid, REG_EXTENDED);
    if(errCode != 0){
        regerror(errCode, &reUseLid, errBuf, maxLineLength);
        fprintf(stderr, "ERROR %s; complied from %s\n", errBuf, patUseLid);
        _Exit(1);
    }
    errCode = regcomp(&reUseSpk, patUseSpk, REG_EXTENDED);
    if(errCode != 0){
        regerror(errCode, &reUseSpk, errBuf, maxLineLength);
        fprintf(stderr, "ERROR %s; complied from %s\n", errBuf, patUseSpk);
        _Exit(1);
    }
    errCode = regcomp(&reLidReport, patLidReport, REG_EXTENDED);
    if(errCode != 0){
        regerror(errCode, &reLidReport, errBuf, maxLineLength);
        fprintf(stderr, "ERROR: %s; pattern: %s\n", errBuf, patLidReport);
        _Exit(1);
    }
    errCode = regcomp(&reSpkReport, patSpkReport, REG_EXTENDED);
    if(errCode != 0){
        regerror(errCode, &reSpkReport, errBuf, maxLineLength);
        fprintf(stderr, "ERROR: %s; pattern: %s\n", errBuf, patSpkReport);
        _Exit(1);
    }
    errCode = regcomp(&reSavePath, patSavePath, REG_EXTENDED);
    if(errCode != 0){
        regerror(errCode, &reSavePath, errBuf, maxLineLength);
        fprintf(stderr, "ERROR: %s; pattern: %s\n", errBuf, patSavePath);
        _Exit(1);
    }
    bool bUseLid = false;
    bool bUseSpk = false;
    bool bHasCheckLid = false;
    bool bHasCheckSpk = false;
    unsigned totalCnt = 0;
    char tmpLine[maxLineLength];
    while(true){
        if(ifs.getline(tmpLine, maxLineLength).eof()){
            struct tm tt;
            time_t ttt;
            time(&ttt);
            localtime_r(&ttt, &tt);
            fprintf(stderr, "%02d:%02d:%02d detect the EOF, but continue to read new content after a while.\n", tt.tm_hour, tt.tm_min, tt.tm_sec);
            ifs.clear();
            ts.tv_sec = 4; 
            select(0, NULL, NULL, NULL, &ts);
            continue;
        }
        printf("from ioacas_.log: %s\n", tmpLine);
        if(ifs.fail()){
            ifs.clear();
        }
        if(bHasCheckLid && bHasCheckSpk){
            if(bUseSpk){
                errCode = regexec(&reSpkReport, tmpLine, 10, subMatches, 0);
                if(errCode == 0){
                    string strPid = csubstr(tmpLine, subMatches[1].rm_so, subMatches[1].rm_eo);
                    string strPath;
                    if(regexec(&reSavePath, tmpLine, 10, subMatches, 0) == 0){
                     strPath = csubstr(tmpLine, subMatches[1].rm_so, subMatches[1].rm_eo);   
                    }
                    fprintf(stdout, "INFO report: PID=%s SAVEDPATH=%s\n", strPid.c_str(), strPath.c_str());
                    unsigned long long uPid = strtoull(strPid.c_str(), NULL, 10);
                    if(recordRecatchInfo(uPid, strPath)){
                        totalCnt++;
                    }
                }
                else if(errCode != REG_NOMATCH){
                    regerror(errCode, &reSpkReport, errBuf, maxLineLength);
                    fprintf(stderr, "ERROR %s; pattern: %s; text: %s\n", errBuf, patSpkReport, tmpLine);
                }
            }
            if(bUseLid){
                errCode = regexec(&reLidReport, tmpLine, 10, subMatches, 0);
                if(errCode == 0){
                    string strPid = csubstr(tmpLine, subMatches[1].rm_so, subMatches[1].rm_eo);
                    string strPath;
                    if(regexec(&reSavePath, tmpLine, 10, subMatches, 0) == 0){
                     strPath = csubstr(tmpLine, subMatches[1].rm_so, subMatches[1].rm_eo);   
                    }
                    fprintf(stdout, "INFO report: lidreg PID=%s SAVEDPATH=%s\n", strPid.c_str(), strPath.c_str());
                    unsigned long long uPid = strtoull(strPid.c_str(), NULL, 10);
                    if(recordRecatchInfo(uPid, strPath)){
                        totalCnt++;
                    }
                }
                else if(errCode != REG_NOMATCH){
                    regerror(errCode, &reLidReport, errBuf, maxLineLength);
                    fprintf(stderr, "ERROR %s; pattern: %s; text: %s\n", errBuf, patLidReport, tmpLine);
                }

            }
            if(totalCnt == g_mId2Infos.size() || totalCnt == 2 * g_mId2Infos.size()){
                break;
            }
            continue;
        }
       // check whether UseLid is set or not. 
        errCode = regexec(&reUseLid, tmpLine, 10, subMatches, 0);
        if(errCode == 0){
            string strToken = csubstr(tmpLine, subMatches[1].rm_so, subMatches[1].rm_eo);
            if(strToken == "1" || strToken== "true" || strToken == "True"){
                bUseLid = true;
            }
            else{
                bUseLid = false;
            }
            bHasCheckLid = true;
        }
        else if(errCode != REG_NOMATCH){
            fprintf(stderr, "ERROR %s; pattern %s; text: %s\n", errBuf, patUseLid, tmpLine);
        }
       // check whether UseSpk is set or not. 
        errCode = regexec(&reUseSpk, tmpLine, 10, subMatches, 0);
        if(errCode == 0){
            string strToken = csubstr(tmpLine, subMatches[1].rm_so, subMatches[1].rm_eo);
            if(strToken == "1" || strToken== "true" || strToken == "True"){
                bUseSpk = true;
            }
            else{
                bUseSpk = false;
            }
            bHasCheckSpk = true;
        }
    }
}
void mycallback(const RESULT_STRUCT &pResults)
{
	//只记录首次回报时间。
#ifdef UNDER_DEV
	ProjInfo *curobjptr = g_mId2Infos[pResults.ID];
	pthread_mutex_lock(&(curobjptr->m_objLock));
	if(curobjptr->recatchtime == 0){
		curobjptr->recatchtime = time(NULL);
	}
	pthread_mutex_unlock(&(curobjptr->m_objLock));
#endif
	if(pResults.isSpeakerResult){
		static int spk_cnt = 0;
		cout << "feed back speaker result with log id: " << pResults.ID <<" ; SPKREG total num: "<< ++spk_cnt <<endl;
		cout<< "    "<< "speaker: " << pResults.speaker<<endl;
		cout<< "    "<< "speakerID: "<<pResults.speakerID<<endl;
		cout<< "    "<<"savePath: "<<pResults.speakerDatasavpath<<endl;
		cout<< "    "<<"speakerFlag: "<<static_cast<unsigned int>(static_cast<unsigned char>(pResults.speakerFlag))<<endl;
		cout<<"    " <<"languageFlag: "<<static_cast<unsigned int>(static_cast<unsigned char>(pResults.languageFlag))<<endl;
		cout<<"    "<<"speakerConfidence: "<<pResults.speakerConfidence<<endl;
	}
	if(pResults.isLangResult){
#ifdef UNDER_DEV
		const char * sepptr = strchr(pResults.langDatasavpath, ':');
		if(sepptr  == NULL){
			curobjptr->postFilePath = pResults.langDatasavpath;
		}
		else{
			curobjptr->postFilePath = sepptr + 1;
		}
#endif
		static int lid_cnt = 0;
		cout << "feed back language result with log id: "<< pResults.ID <<" ; total num: "<< ++lid_cnt <<endl;
		cout<< "    "<<"language: "<<pResults.language<<endl;
		cout<< "    "<<"langID: " <<pResults.langID<<endl;
		cout<< "    "<<"savePath: "<<pResults.langDatasavpath<< endl;
	}
	if(!pResults.isSpeakerResult && !pResults.isLangResult){
		//static int nores_cnt = 0;
		cout<< "<"<< pthread_self() <<"> WARN feed back no result with log id: "<< pResults.ID<<endl;
	}
}

static void saveSendList()
{
    fstream of(g_sendInfoFile, fstream::out);
	if(!of.is_open()){
		cerr<< "can't open file: " << g_sendInfoFile << ", for saving list send." << endl;
		return;
	}
    for(map<unsigned long long, ProjInfo*>::const_iterator it=g_mId2Infos.begin(); it!=g_mId2Infos.end(); it++){
		of<<it->second->toString()<<endl;
	}
	of.close();
	cout<< "have saved information of audio waves send in file "<< g_sendInfoFile <<" total: "<< g_mId2Infos.size()<< endl;
}

char *readModel(const char* file, unsigned &len)
{
	const unsigned ALLOC_UNIT = 1024;
	FILE *fp = fopen(file, "rb");
	if(fp == NULL){
		cerr <<"can't open model file. file: "<< file<< endl;
		return NULL;
	}
	unsigned step = 0;
	unsigned curSize = 0;
	char * retPtr = NULL;
	do{
        retPtr = (char*)realloc(retPtr, curSize + ALLOC_UNIT);
		if(retPtr == NULL){
			cerr << "fail to allocate memory. size: "<< curSize+ALLOC_UNIT <<endl;
			curSize = 0;
			break;
		}
		step = fread(retPtr+curSize, 1, 1024, fp);
		curSize += step;
	}while(step == ALLOC_UNIT);
	fclose(fp);
	len = curSize;
	return retPtr;
}

int addMySpeakers()
{
	const char renderDir[] = "./SpkModel";
	DIR * dp;
	struct dirent *dirp;
	if((dp=opendir(renderDir)) == NULL){
		cerr << "can't open the directory for reading models. dir: " << renderDir<< endl;
		return 0;
	}
	unsigned accul = 0;
	while((dirp=readdir(dp)) != NULL){
		int spkId = 0;
		unsigned char serviceFlag = 0;
		short harmLevel = 0;
		if(sscanf(dirp->d_name, "%d_%x_%d.param", &spkId, &serviceFlag, &harmLevel) == 3){
            cout << "parepare to send one speaker model, id: "<< spkId<<"service flag: "<< (unsigned)serviceFlag << "; harmLevel: " <<harmLevel << ".\n";
			char filePath[512];
			sprintf(filePath, "%s/%s", renderDir, dirp->d_name);
			char *modelData = NULL;
			unsigned dataLen = 0;
			if((modelData=readModel(filePath, dataLen)) != NULL){
               if(AddSpeaker(spkId, modelData, dataLen, serviceFlag, harmLevel) != -1){
				   //save data through interface for debug.
				   char dpname[20];
				   sprintf(dpname, "%d", spkId);
				   FILE *dpfp=fopen(dpname, "wt");
				   if(dpfp != NULL){
						fwrite(modelData, 1, dataLen, dpfp);
						fclose(dpfp);
				   }
				   accul ++;
			   } 
			}
		}
	}
	closedir(dp);
	return accul;
}
namespace BufferGlobal{

void notifyProjFinish(unsigned long long);
};

/**
 * 从模式*_<pid>_*中提取出节目名，当节目名唯一时，提取成功，否则，提取失败。
 */
bool parse_allpaths_pids(const vector<string>& allpaths, vector<unsigned long long>& accordpids)
{
	accordpids.clear();
	for(size_t idx=0; idx < allpaths.size(); idx++){
		string file = allpaths[idx];
		size_t fipl = file.find_first_of("_");
		if(fipl == string::npos) break;
		size_t sepl = file.find_first_of("_", fipl+1);
		if(sepl == string::npos) break;
		string strpid = file.substr(fipl + 1, sepl - fipl -1);
		unsigned long long ullvar;
		if(sscanf(strpid.c_str(), "%llu%", &ullvar) != 1) break;
		accordpids.push_back(ullvar);
	}
	if(accordpids.size() != allpaths.size()){
		accordpids.clear();
	}
	set<unsigned long long> pidsSet;
	for (unsigned u=0; u< accordpids.size(); u++){
		if(pidsSet.insert(accordpids[u]).second == false){
			accordpids.clear();
			break;
		}
	}
	return accordpids.size() > 0;
}
/**
 * 若首次执行该函数，就把rt目录下的wav文件读出；后面执行此函数，就把上次读出的删除并把新的文件代替，继续下一轮执行。
 * 继续与否可以通过与用户交互得知。
 *
 */
static size_t fillTaskInfosGlobal(const char *rt)
{
	for(map<unsigned long long, ProjInfo*>::iterator it=g_mId2Infos.begin(); it!=g_mId2Infos.end(); it++){
		ProjInfo *its = it->second;
		if(its->postFilePath != ""){
			size_t lastdotidx = its->postFilePath.find_last_of(".");
			string newFilePath = its->postFilePath.substr(0, lastdotidx) + ".wr" + its->postFilePath.substr(lastdotidx);
			string mvcmd = (string)"mv -f " + its->postFilePath + " " + newFilePath;
			fprintf(stdout, "INFO: start moving audiofile %s --> %s\n", its->postFilePath.c_str(), newFilePath.c_str());
			int status = system(mvcmd.c_str());
			if(status == -1 || !WIFEXITED(status)){
				fprintf(stderr, "WARNING: fail move the saved result file to source dir. %S:%d", __FILE__, __LINE__);
			}
		}
	}
	for(map<unsigned long long, ProjInfo*>::iterator it=g_mId2Infos.begin(); it!=g_mId2Infos.end(); it++){
		delete it->second;
		it->second = NULL;
	}
	g_mId2Infos.clear();
	vector<string> allwavs = get_file_list(rt);
	vector<unsigned long long> parsedpids;
	parse_allpaths_pids(allwavs, parsedpids);
	for(int i=0; i<allwavs.size(); i++){
		ProjInfo* up = new ProjInfo();
		long id = up->id;
		if(parsedpids.size() > 0) id = up->id = parsedpids[i];
		up->filePath = (string)rt + allwavs[i];
		g_mId2Infos[id] = up;
	}
	return g_mId2Infos.size();
}
#include <unistd.h>
#include "myiofuncs.h"
void fetch_all_options(int argc, char* argv[], char *opts, map<char,  string> &optbox)
{
	opterr = 1;
	char c;
	while( (c = getopt(argc, argv, opts)) != EOF){
		switch (c){
			case '?':
				fprintf(stderr,"usage: main -m 1 -p 10 -d wavedir/\n");
				exit(1);
				break;
			default:
				if(optarg == NULL) optbox[c] = "";
				else
					optbox[c] = optarg;
		}
	}
}

#define SAMPLE_RATE 8000
int main(int argc, char *argv[])
{
	/* usage: main -m <seconds> -d <wavedir/> -p <threads> -k <saved list>
	 * -m <seconds> when <seconds> is -1, use random seconds serial following a buildin distribution; otherwise, use <seconds> seconds interval to send package.
	 *
	 */
	bool isconti = true;
	map<char, string> allopts;
	fetch_all_options(argc ,argv, "bm:d:p:k:s:", allopts);
	printf("debug: %s\n", map2str(allopts).c_str());
	char rt[512];
	int parallelNum = 1;
	bool useRand = false;
	sprintf(rt, "%s", "/home/ioacas/testWave/");
	sprintf(g_sendInfoFile, "%s", "output/send.list");
	if(allopts.find('d') != allopts.end()){
		sprintf(rt, "%s", allopts['d'].c_str());
	}
	if(allopts.find('p')!= allopts.end()){
		parallelNum = atoi(allopts['p'].c_str());
	}
	if(allopts.find('m')!= allopts.end()){
		g_intervalSeconds = atoi(allopts['m'].c_str());
	}
	if(allopts.find('k') != allopts.end()){
		sprintf(g_sendInfoFile, "%s", allopts['k'].c_str());
	}
	if(allopts.find('s') != allopts.end()){
		g_packetBytes = atoi(allopts['s'].c_str())* SAMPLE_RATE * 2;
	}
	if(allopts.find('b') != allopts.end()){
		isconti = true;
	}
	else {
		isconti = false;
	}
	size_t rtlen = strlen(rt);
	if(rt[rtlen -1] != '/'){
		rt[rtlen] = '/';
		rt[++rtlen] = 0;
	}
	//if(atexit(saveSendList) != 0){
	//	fprintf(stderr, "error: fail to register exiting function savesendlist.\n");
	//}
    int retInt= Ioacas_Initialize((unsigned long)mycallback, 1);
	if(retInt != 0){
		cerr<< "fail initialize ioacas"<< endl;
		abort();
	}
    g_ifsIoacasLog.open(g_szLogFile, std::ifstream::in);
    if(!g_ifsIoacasLog.is_open()){
        fprintf(stderr, "error: failed open file %s.\n", g_szLogFile);
    }
    g_ifsIoacasLog.seekg(0, g_ifsIoacasLog.end);
    while(true){
        if(Ioacas_IsRecReady()){
            break;
        }
        printf("warn: ioacas module is not ready for processing data, then sleep for 2 seconds and try again.\n");
        sleep(2);
    }

	size_t audionum  = 0;
	char tmpstr[256];
	struct timeval tv;
	tv.tv_sec = 1;
	tv.tv_usec = 0;
	char *retfgets;
	RemoveAllSpeaker();
	addMySpeakers();
	do{
		audionum = fillTaskInfosGlobal(rt);
		fprintf(stdout, "having read %d tasks, before starting this round, please strike key s to stop, or continue after 3 secs:", audionum);
		retfgets = fgetstimeout(tmpstr, 256, 3);
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
		if(audionum == 0){
			continue;
		}
		send_data_parallel(parallelNum);
        saveSendList();
        //get the progress for projects by inspecting log file ./ioacas/ioacas_.log.
        monitorRecognitionProgress();
#ifdef UNDER_DEV
		//wait all results returned.
		for(std::map<unsigned long long, ProjInfo*>::iterator it= g_mId2Infos.begin(); it != g_mId2Infos.end(); it++){
			long int testlen = it->second->sendLen;	
			bool needwait = testlen >= MIN_VALID_WAVE_LEN;
			fprintf(stdout, "checking task for waiting. %llu --- %s\n", it->first, it->second->toString().c_str());
			while(needwait){
				tv.tv_sec = 10;//sleep 10 secs.
				select(0, NULL, NULL, NULL, &tv);
				pthread_mutex_lock(&it->second->m_objLock);
				time_t testtime = it->second->recatchtime;
				pthread_mutex_unlock(&it->second->m_objLock);
				if(testtime != 0) break;
			}
		}
#endif
        //rewrite the backup file with data having something new.
        saveSendList();
	}while(isconti);
	
	cout<< ">>>>>>>>>>>>>finish test<<<<<<<<<<<<<<<\n";

	Ioacas_ShutDown();
	return 0;
}
