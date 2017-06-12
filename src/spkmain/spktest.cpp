/*************************************************************************
    > File Name: spktest.cpp
    > Author: zhengshurui
    > Mail:  zhengshurui@thinkit.cn
    > Created Time: Wed 12 Oct 2016 06:39:04 PM PDT
 ************************************************************************/

#include <unistd.h>
#include <cstdlib>
#include <cstdio>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>

//#include "../dllSRVADCluster.h"
//#include "../MusicDetect_dup.h"
#include "../utilites.h"
#ifdef TEST_SPKEX
#include "../spk_ex.h"
#else
#include "../interface.h"
#endif

using namespace std;

#define SC_PCMSMPS 8000
#define VALID_PCMSMPS 10 * SC_PCMSMPS
static const unsigned MAX_DATA_LEN = 60 * 60 * 16000;// one hour pcm.

char *g_szAudioListFile = NULL;
char *g_szResultListFile = NULL;
char *g_szModelListFile = NULL;
bool g_bDebugMode = false;
#define DEBUG_LOG(FMT, ...) if(g_bDebugMode) fprintf(stderr, FMT "\n", ##__VA_ARGS__);
#define ERROR_LOG(FMT, ...) fprintf(stderr, "ERROR " FMT "\n", ##__VA_ARGS__);

#ifdef TEST_SPKEX
class SpkInfoEx: public SpkInfo
{
    static unsigned long acculCnt;
public:
    SpkInfoEx(const char* param, char* buf = NULL, unsigned len = 0):
        SpkInfo(acculCnt ++)
    {
        fname = param;
        size_t idx = fname.find_last_of('/');
        if(idx != string::npos){
            fname = fname.substr(idx + 1);
        }
        mdData = buf;
        mdLen = len;
    }
    string toStr() const{
        return fname;
    }
    bool fromStr(const char* param)
    {
        fname =param;
        return true;
    }

    string fname;
    char *mdData;
    unsigned mdLen;
};
unsigned long SpkInfoEx::acculCnt = 0;
#endif

bool loadBinary2Buf(const char* path, char *pData, unsigned &iolen)
{
    ifstream istm(path, ifstream::binary);
    if(!istm){
        cerr << "error load data from file: "<< path<< std::endl;
        return false;
    }
    bool ret =true;
    istm.seekg(0, istm.end);
    int len = istm.tellg();
    istm.seekg(0, istm.beg);
    if(pData == NULL){
        iolen = len;
    }
    else{
        if(len > iolen) len = iolen;
        istm.read(pData, len);
        iolen  = len;
        if(!istm){
            cerr<< "error reading block from file: "<< path<<", size: "<< len<< ", read: "<< istm.gcount()<< ".\n";
            iolen = istm.gcount();
            ret = false;
        }
    }
    istm.close();
    return ret;
    
}
bool loadBinaryFile(const char* path, char *&mdData, unsigned &mdLen)
{
    if(!loadBinary2Buf(path, NULL, mdLen)){
        return false;
    }
    mdData = new char[mdLen];
    if(!loadBinary2Buf(path, mdData, mdLen)){
        return false;
    }
    return true;
}

#ifdef TEST_SPKEX
vector<SpkInfoEx*> getallMdls(const char* listfile)
{
    vector<SpkInfoEx*> ret;
    vector<string> vecPath = loadFileList(listfile);
    for(size_t idx=0; idx < vecPath.size(); idx ++){
        char *mdData;
        unsigned mdLen;
        loadBinaryFile(vecPath[idx].c_str(), mdData, mdLen);
        SpkInfoEx* pSpk = new SpkInfoEx(vecPath[idx].c_str(), mdData, mdLen);
        ret.push_back(pSpk);
    }
    return ret;
}
#endif

bool prepareBufAndData(const char* fpath, unsigned &dataLen, char*&pData, char*& addBuf1, char* &addBuf2)
{
    static char dataBuf[MAX_DATA_LEN];
    static char addbuf_1[MAX_DATA_LEN];
    static char addbuf_2[MAX_DATA_LEN];
    dataLen = MAX_DATA_LEN;
    pData = dataBuf;
    addBuf1 = addbuf_1;
    addBuf2 = addbuf_2;

    if(!loadBinary2Buf(fpath, pData, dataLen)){
        return false;
    }
    if(dataLen > 44){
        dataLen -= 44;
        pData += 44;
    }

    return true;
}

void parseGlobal(int argc, char *argv[])
{
	while(true){
		int retc = getopt(argc, argv, "c:f:t:d");
		if(retc == -1) break;
		switch(retc){
			case('c'):
                g_szModelListFile = optarg;
				break;
			case('f'):
				g_szAudioListFile = optarg;
				break;
			case('t'):
				g_szResultListFile  = optarg;
				break;
			case('d'):
				g_bDebugMode = true;
			default:
				break;
		}
	}
	if(g_szAudioListFile == NULL || g_szModelListFile == NULL || g_szResultListFile == NULL){
		ERROR_LOG( "usage: program <-d> -f audiolist -t resultlist -c modellist.\n\t-d\tdebug mode");
	}
    if(g_szAudioListFile == NULL){
        g_szAudioListFile = new char[MAX_PATH];
        snprintf(g_szAudioListFile, MAX_PATH, "audiolist");
    }
    if(g_szModelListFile == NULL){
        g_szModelListFile = new char[MAX_PATH];
        snprintf(g_szModelListFile, MAX_PATH, "modellist");
    }
    if(g_szResultListFile == NULL){
        g_szResultListFile = new char[MAX_PATH];
        snprintf(g_szResultListFile, MAX_PATH, "resultlist");
    }
	DEBUG_LOG("cfgFile: %s;\nfromFile: %s;\ntoFile: %s;\ndebugMode: %d", g_szModelListFile, g_szAudioListFile, g_szResultListFile, g_bDebugMode);
}

#ifndef TEST_SPKEX
static const void *g_SpkMdls[100];
static vector<string> g_vecPath;
void init_and_loadmodel()
{
    assert(TIT_SPKID_INIT("ioacas/runSpk.cfg") == StsNoError);
    g_vecPath = loadFileList(g_szModelListFile);
    assert(g_vecPath.size() > 0);
    assert(g_vecPath.size() < 100);
    for(unsigned idx=0; idx < g_vecPath.size(); idx++){
        void *tmpptr;
        assert(TIT_SPKID_LOAD_MDL_IVEC(tmpptr, g_vecPath[idx].c_str()) == StsNoError);
        g_SpkMdls[idx] = tmpptr;
    }
}
void spk_score(short* data, unsigned len, char *log, unsigned &offset)
{
    unsigned spknum = g_vecPath.size();
    float *scores = new float[100];
    TITStatus err = TIT_SPKID_VERIFY_CLUSTER(data, len, g_SpkMdls, spknum, scores);
    assert(err == StsNoError);
    for(unsigned idx=0; idx < spknum; idx++){
        offset += sprintf(log+offset, " %0.0f", scores[idx]);
    }
    delete [] scores;
}
#endif

static bool savebinary(char *mdlData, unsigned mdllen, const char *fname)
{
    FILE *fp = fopen(fname, "w");
    if(fp == NULL){
        return false;
    }
    bool ret = true;
    if(fwrite(mdlData, 1, mdllen, fp) != mdllen){
        ret = false;
    }
    fclose(fp);
    return ret;
}

int main(int argc, char *argv[])
{
    string sysdir = "ioacas/";
    parseGlobal(argc, argv);
    //initailize and load models
    #ifdef TEST_SPKEX
    vector<SpkInfoEx*> vecSpks = getallMdls(g_szModelListFile);
    if(vecSpks.size() == 0){
        ERROR_LOG("no spks being configured!!!");
        exit(1);
    }
    else{
        DEBUG_LOG("finish loading spks, count: %lu.", vecSpks.size());
    }
    if(!spkex_init("ioacas/runSpk.cfg")){
        exit(1);
    }
    for(size_t idx = 0; idx < vecSpks.size(); idx++){
        if(!spkex_addSpk(vecSpks[idx], vecSpks[idx]->mdData, vecSpks[idx]->mdLen)){
            ERROR_LOG("fail to add speaker. file: %s", vecSpks[idx]->fname.c_str());
            exit(1);
        }
    }
    #else
    init_and_loadmodel();
    #endif

    
    FILE *resfp = NULL;
    vector<string> vecPcms = loadFileList(g_szAudioListFile);
    if(vecPcms.size() == 0) goto exit_main;
    char szLog[1024];
    
    resfp = fopen(g_szResultListFile, "w");
    if(resfp == NULL){
        ERROR_LOG("fail to open file for storing result. file: %s", g_szResultListFile);
    }
    for(size_t idx=0; idx < vecPcms.size(); idx ++){
        unsigned uLogLen = 0;
        while(true){
            uLogLen += snprintf(szLog + uLogLen, 1024, "SPKREC file: %s ", vecPcms[idx].c_str());
            unsigned dataLen;
            char *pData, *addBuf1, *addBuf2;
            if(!prepareBufAndData(vecPcms[idx].c_str(), dataLen, pData, addBuf1, addBuf2)){
                uLogLen += snprintf(szLog + uLogLen, 1024, "NO_DATA ");
                break;
            }
            unsigned smpLen = dataLen /2;
            short* smpBuf = reinterpret_cast<short*>(pData);
            short *smpAddBuf1 = reinterpret_cast<short*>(addBuf1);
            short *smpAddBuf2 = reinterpret_cast<short*>(addBuf2);
            uLogLen += snprintf(szLog + uLogLen, 1024, "DataSecs=%u ", smpLen / SC_PCMSMPS);

            #ifdef TEST_SPKEX
            const SpkInfo* tspk;
            float spkScore;
            int retscr = spkex_score(smpBuf, smpLen, tspk, spkScore);
            if(tspk == NULL){
                if(retscr == 0) uLogLen += snprintf(szLog + uLogLen, 1024, " NO_SPK ");
            }
            else{
                uLogLen += snprintf(szLog + uLogLen, 1024, " speaker=%s score=%0.2f ", tspk->toStr().c_str(), spkScore);
                returnSpkInfo(tspk);
            }
            #else
            /*
            static unsigned curidx = 0;
            char tmpname[100];
            sprintf(tmpname, "debug/%d.pcm", curidx);
            curidx ++;
            savebinary(reinterpret_cast<char*>(smpBuf), smpLen * 2, tmpname);
            */
            spk_score(smpBuf, smpLen, szLog, uLogLen);           
            #endif
            break;
        }
        DEBUG_LOG("%s", szLog);
        if(resfp){
            fprintf(resfp, "%s\n", szLog);
        }
    }
    if(resfp) fclose(resfp);

exit_main:
    #ifdef TEST_SPKEX
    spkex_rlse();
    #else
    TIT_SPKID_EXIT();
    #endif
    /*
    FreeVADCluster();
    closeMusicCut(hMcut);
    */
}

