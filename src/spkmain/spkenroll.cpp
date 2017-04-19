/*************************************************************************
	> File Name: spkenroll.cpp
	> Author: 
	> Mail: 
	> Created Time: Wed 15 Mar 2017 01:43:13 AM EDT
 ************************************************************************/

#include <unistd.h>
#include <cstdlib>
#include <cstdio>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>

#include "../wav/wav.h"
#include "../utilites.h"
#include "../TIT_SPKID_OFFL_API.h"
#include<iostream>
using namespace std;

#define MAX_PATH 512

char *g_szCfgFile = NULL;
char *g_szInputFiles = NULL;
char *g_szOutFile = NULL;
char g_szModelDir[MAX_PATH];

#define DEBUG_LOG(FMT, ...) if(g_bDebugMode) fprintf(stderr, FMT "\n", ##__VA_ARGS__);
#define ERROR_LOG(FMT, ...) fprintf(stderr, "ERROR " FMT "\n", ##__VA_ARGS__);

bool saveSpkMdl(const char* mdlfile, void *data, unsigned size)
{
    FILE *fp = fopen(mdlfile, "wb");
    if(fp == NULL){
        ERROR_LOG("failed to open file %s", mdlfile);
    }
    fwrite(data, 1, size, fp);
    fclose(fp);
    return true;
}
bool parseGlobal(int argc, char *argv[])
{
    while(true){
        int retc = getopt(argc, argv, "c:i:o:");
        if(retc == -1) break;
        switch(retc){
            case('c'):
            g_szCfgFile = optarg;
            break;
            case('i'):
            g_szInputFiles = optarg;
            break;
            case('o'):
            g_szOutFile = optarg;
            default:
            break;
        }
    }
    if(g_szCfgFile == NULL || g_szInputFiles == NULL || g_szOutFile == NULL){
        ERROR_LOG("usage: projram -c <cfg_file> -i <input_files> -o <model_path>");
        return false;
    }
    snprintf(g_szModelDir, MAX_PATH, ".");
    return true;
}

int main(int argc, char *argv[])
{
    if(!parseGlobal(argc, argv)) exit(1);
    int mdlsize, featsize, indexsize;
    TIT_RET_CODE err = TIT_SPKID_Init(g_szCfgFile, g_szModelDir, featsize, mdlsize, indexsize);
    if(err != TIT_SPKID_SUCCESS){
        ERROR_LOG("failed to tit_spkid_init, TitError: %d", err);
        exit(1);
    }

    vector<Wavs> wavs;
    char *st = g_szInputFiles;
    bool bFinished = false;
    while(!bFinished){
        string curfile;
        char *del = strchr(st, ',');
        if(del == NULL){
            curfile = st;
            bFinished = true;
        }
        else{
             curfile = string(st, del);   
            st = del + 1;
        }
        
        short *wavbuf1= NULL;
        int wavlen1 = 0;
        short *wavbuf2 = NULL;
        int wavlen2 = 0;
        int nchl;
        if(!CreateWav(curfile.c_str(), nchl, wavbuf1, wavlen1, wavbuf2, wavlen2)){
            ERROR_LOG("failed to load wave from file: %s.", curfile.c_str());
            continue;
        }
        wavs.push_back(Wavs());
        wavs.back().wavbuf = wavbuf1;
        wavs.back().wavlen = wavlen1;
        DestroyWav(wavbuf2, wavlen2);
    }
    if(wavs.size() == 0){
        ERROR_LOG("no wave data left to enroll speaker %s.", g_szOutFile);
    }

    void *spkmdl = malloc(featsize);
    err = TIT_TRN_Model_CutAll(&wavs[0], wavs.size(), spkmdl);
    if(err != TIT_SPKID_SUCCESS){
        ERROR_LOG("failed to tit_trn_model_cutall, TitError: %d", err);
        exit(1);
    }
    saveSpkMdl(g_szOutFile, spkmdl, featsize);
    free(spkmdl);
    TIT_SPKID_Exit();
}
