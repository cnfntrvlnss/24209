/*************************************************************************
	> File Name: libBAI_fork.cpp
	> Author: 
	> Mail: 
	> Created Time: Thu 09 Jun 2016 11:12:06 PM CST
 ************************************************************************/

#include <cstdlib>
#include <cstring>
#include<iostream>
using namespace std;

#include "libBAI.h"

BAI_InputItem::BAI_InputItem()
{
    pcDataBuffer = NULL;
    iBufferSize = 0;
}
BAI_InputItem::~BAI_InputItem()
{
    if(pcDataBuffer != NULL){
        delete []pcDataBuffer;
    }
}
BAI_ResultItem::BAI_ResultItem()
{
    eErrCode = BAI_OK;
    iLibraryID = 0;
    acAudioUrl[0] = '\0';
    fMatchedRate = 0.0;
    fTimeStartInTestS = 0.0;
    fTimeStartInWaveS = 0.0;
    fDurationS = 0.0;
}

BAI_ResultList::BAI_ResultList()
{
    eErrCode = BAI_OK;
    iTestID = 0;
    iResultNum = 0;
    pstResultItems = NULL;
}
BAI_ResultList::~BAI_ResultList()
{
    if(pstResultItems != NULL){
        delete pstResultItems;
    }
}

BAI_Code BAI_Init(const char* cfgfile, const int thrdNum)
{
    return BAI_OK;
}

void * oneHdl = reinterpret_cast<void*>(static_cast<unsigned long>(1));

BAI_Code BAI_Open(void **ppThreadIdx)
{
    return BAI_OK;
}

BAI_Code BAI_Close(void **ppThreadIdx)
{
    return BAI_OK;
}
BAI_Code BAI_Exit()
{
    return BAI_OK;
}

BAI_Code BAI_ExtractDNA(BAI_InputItem* inputItems, const int inputNum, const int dnaType)
{
    return BAI_OK;
}

BAI_Code BAI_BuildIndex(BAI_InputItem* inputItems, const int inputNum, const char * hashIdx, const int thdNum)
{
    return BAI_OK;
}

BAI_Code BAI_LoadIndex(const char* hashFile, void** ppThreadIdx)
{
    return BAI_OK;
}

BAI_Code BAI_Retrieval_Partly(BAI_InputItem* inputItems, const int inputNum, BAI_ResultList*& resList, void** thdIdx)
{
    fprintf(stderr, "\e[0;31mWARN fork function of BAI_Retrieval_Partly.\e[0m\n");
    if(rand()%100 < 0){
        return BAI_OK;
    }
    resList = new BAI_ResultList[inputNum];
for(int i=0; i< inputNum; i++){
    resList[i].iResultNum = 1;
    resList[i].iTestID = i;
    resList[i].pstResultItems = new BAI_ResultItem;
    BAI_ResultItem &item = *resList[i].pstResultItems;
    item.iLibraryID = 6941;
    //strcpy(item.acAudioUrl, "fork sample");
    sprintf(item.acAudioUrl, "%d", 6941);
    item.fMatchedRate = 100.0;
    item.fTimeStartInTestS = 0.0;
    item.fTimeStartInWaveS = 0.0;
    item.fDurationS = 3.0;

}
    return BAI_OK;
}
BAI_Code BAI_Retrieval_Partly_VAD(BAI_InputItem* inputItems, const int inputNum, BAI_ResultList*& resList, void** thdIdx)
{
    return BAI_Retrieval_Partly(inputItems, inputNum, resList, thdIdx);
}

