/*************************************************************************
  > File Name: TLI_API_fork.cpp
  > Author: 
  > Mail: 
  > Created Time: Thu 12 Jan 2017 07:41:57 PM EST
 ************************************************************************/

#include "dllVAD.h"
#include "TLI_API.h"
#include <cstdio>
#include <cstring>
#include<iostream>
using namespace std;

#define FORKOP(x) fprintf(stderr, "faked function " #x " in %s.\n", __FILE__);


bool DLLLRVAD_API InitializeVADs(const VADCfg& rsVADCfg)
{
    FORKOP(InitializeVADs);
    return true;
}

bool DLLLRVAD_API ExtractSpeechBuf(const int iThreadIdx, const short* psPCMBuffer, const int iSampleNumber, short*& psPCMBufferVAD, int& iSampleNumVAD){
    FORKOP(ExtractSpeechBuf);
    if(iSampleNumVAD > iSampleNumber) iSampleNumVAD = iSampleNumber;
    memcpy(psPCMBufferVAD, psPCMBuffer, sizeof(short) * iSampleNumVAD);
    return true;
}

bool DLLLRVAD_API FreeVADs()
{
    FORKOP(FreeVADs);
    return true;
}

int TLI_Init(            
        char * pszTLISystemDirectory,
        int * pnAllTemplateIDs,
        char ** ppszAllTemplateLocation,
        int nAllTemplateNumber,
        int nLineNumber
        )
{
    FORKOP(TLI_Init);
    return 0;
}

int TLI_Exit()
{
    FORKOP(TLI_Exit);
    return 0;
}

int TLI_Open(
	TLI_HANDLE & hTLI
)
{
    FORKOP(TLI_Open);
    hTLI = 0;
    return 0;
}
int TLI_Close(
	TLI_HANDLE hTLI
)
{
    FORKOP(TLI_Close);
    return 0;
}
int TLI_Recognize(
	TLI_HANDLE hTLI,
	int * pnCurrentTemplateIDs,
	int nCurrentTemplateNumber,
	void * pvPCMBuffer,
	int nBytesOfBuffer,
	int nMinLimit,
	int nMaxLimit
    ){
    FORKOP(TLI_Recognize);
    return 0;
}
int TLI_GetResult(
	TLI_HANDLE hTLI,
	float * pfResults,
	int nNumber
)
{
    FORKOP(TLI_GetResult);
    for(int i=0; i< nNumber; i++){
        pfResults[i] = 0.0;
    }
    return 0;
}

