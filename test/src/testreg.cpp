/*************************************************************************
    > File Name: testreg.cpp
    > Author: zhengshurui
    > Mail:  zhengshurui@thinkit.cn
    > Created Time: Thu 21 Jul 2016 05:54:47 PM PDT
 ************************************************************************/

#include<string.h>
#include<stdio.h>
#include <regex.h>
#include<iostream>
#include<fstream>
#include<string>
using namespace std;

const char *g_szLogFile = "../ioacas/ioacas_.log";
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
void monitorRecognitionProgress()
{
    ifstream ifs(g_szLogFile);
    if(!ifs.good()){
        fprintf(stderr, "ERROR fail to open file %s\n", g_szLogFile);
        return;
    }
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
    const char patLidReport[] = "LIDREG PID=([0-9]+) WavLen=";
    regex_t reUseLid, reUseSpk, reLidReport, reSpkReport;
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
    bool bUseLid = false;
    bool bUseSpk = false;
    bool bHasCheckLid = false;
    bool bHasCheckSpk = false;
    char tmpLine[maxLineLength];
    while(true){
        if(ifs.getline(tmpLine, maxLineLength).eof()){
            fprintf(stderr, "detect the EOF, but continue to read new content after a while.\n");
            ifs.clear();
            ts.tv_sec = 2;// sleep 2 seconds.
            select(0, NULL, NULL, NULL, &ts);
            continue;
        }
        if(ifs.fail()){
            ifs.clear();
        }
        if(bHasCheckLid && bHasCheckSpk){
            if(bUseSpk){
                errCode = regexec(&reSpkReport, tmpLine, 10, subMatches, 0);
                if(errCode == 0){
                    string strPid = csubstr(tmpLine, subMatches[1].rm_so, subMatches[1].rm_eo);
                    string strPath;
                    //if(subMatches[3].rm_so != -1) strPath  = csubstr(tmpLine, subMatches[3].rm_so, subMatches[3].rm_eo);
                    fprintf(stdout, "INFO report: spkreg PID=%s SAVEDPATH=%s\n", strPid.c_str(), strPath.c_str());
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
                    //if(subMatches[3].rm_so != -1) strPath  = csubstr(tmpLine, subMatches[3].rm_so, subMatches[3].rm_eo);
                    fprintf(stdout, "INFO report: lidreg PID=%s SAVEDPATH=%s\n", strPid.c_str(), strPath.c_str());
                }
                else if(errCode != REG_NOMATCH){
                    regerror(errCode, &reLidReport, errBuf, maxLineLength);
                    fprintf(stderr, "ERROR %s; pattern: %s; text: %s\n", errBuf, patSpkReport, tmpLine);
                }
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
            printf("INFO haved gotten lid indicator: %d.\n", bUseLid);
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
            printf("INFO haved gotten spk indicator: %d.\n", bUseSpk);
        }
    }
}

int main()
{
    monitorRecognitionProgress();
}
