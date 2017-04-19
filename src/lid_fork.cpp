/*************************************************************************
	> File Name: lid_fork.cpp
	> Author: 
	> Mail: 
	> Created Time: Sun 27 Nov 2016 03:23:08 AM EST
 ************************************************************************/

#include "MusicDetect.h"
#include "TLI_API.h"
#include "dllVAD.h"
#include<iostream>
using namespace std;

int TLI_Init(char * pszTLISystemDirectory, int * pnAllTemplateIDs, char ** ppszAllTemplateLocation, int nAllTemplateNumber, int nLineNumber)
{
    return 0;
}
int TLI_Exit()
{
    return 0;
}

int TLI_Open( TLI_HANDLE & hTLI)
{
    hTLI = 0;
    return 0;
}
int TLI_Close(TLI_HANDLE hTLI)
{
    return 0;
}
int TLI_Recognize(
	TLI_HANDLE hTLI,
	int * pnCurrentTemplateIDs,
	int nCurrentTemplateNumber,
	void * pvPCMBuffer,
	int nBytesOfBuffer,
	int nMinLimit,
	int nMaxLimit)
{
    return 0;
}
int TLI_GetResult(
	TLI_HANDLE hTLI,
	float * pfResults,
	int nNumber)
{
    return -1;  
}
bool InitializeVADs(const VADCfg& rsVADCfg)
{
    return true;
}

bool ExtractSpeechBuf(const int iThreadIdx, const short* psPCMBuffer, const int iSampleNumber, short*& psPCMBufferVAD, int& iSampleNumVAD)
{
    return false;
}

bool FreeVADs()
{
    return true;
}
bool MusicCut_Initial(char* CfgFile,int ThreadNum)
{
    return true;
}
bool MusicCut(int ThreadIndex,short* src,int iLen,short* &dst,int &oLen)
{
    return false;
}
bool MusicCut_Free()
{
    return true;
}
