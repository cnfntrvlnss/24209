/*************************************************************************
    > File Name: spkvad.cpp
    > Author: zhengshurui
    > Mail:  zhengshurui@thinkit.cn
    > Created Time: Tue 15 Mar 2016 08:03:14 PM PDT
 ************************************************************************/
#include <cstdlib>
#include<iostream>
using namespace std;

#include <unistd.h>
//#include "../commonFunc.h"
#include "../dllSRVADCluster.h"
#include "../MusicDetect.h"
#include "../waveinfo.h"

bool bDebugMode = false;
#define DEBUG_OUTPUT(FMT, ...) if(bDebugMode) fprintf(stdout, FMT"\n", ##__VA_ARGS__);
#define ERROR_OUTPUT(FMT, ...) fprintf(stderr, "ERROR "FMT"\n", ##__VA_ARGS__);
#define MAX_PATH 512
char szSpkVADCfg[MAX_PATH] = "VAD_SID.cfg";
char szMusicDetectCfg[MAX_PATH] = "Music.cfg";
#ifdef NO_MUSIC_LIB
bool MusicCut_Initial(char* CfgFile,int ThreadNum)
{
	return true;
}
bool MusicCut(int ThreadIndex,short* src,int iLen,short* &dst,int &oLen)
{
	int trLen = iLen > oLen ? oLen : iLen;
	memcpy(dst, src, trLen);
	return true;
}
bool MusicCut_Free()
{
	return true;
}

#endif
static bool initEngines(bool bUseMusic=false, bool bUseVAD=true)
{
	if(bUseVAD){
		 if(InitVADCluster(szSpkVADCfg)){
			 DEBUG_OUTPUT( "InitVADCluster successfully!");
		 }
		 else{
			 ERROR_OUTPUT( "InitVADCluster failed!");
			 return false;
		 }
	}

	if(bUseMusic){
		if(MusicCut_Initial(szMusicDetectCfg,1)){
			DEBUG_OUTPUT( "MusicCut_Initial Successfully!");
		}
		else
		{
			ERROR_OUTPUT( "MusicCut_Initial Failed!");
			FreeVADCluster();
			return false;
		}
	}
	return true;
}
static bool freeEngines(bool bUseMusic = false, bool bUseVAD= true)
{
	if(bUseVAD){
		if(!FreeVADCluster()){
			ERROR_OUTPUT( "FreeVADCluster failed!");
		}
		else{
			DEBUG_OUTPUT("FreeVADCluster succeeded!");
		}
	}
	if(bUseMusic)
		MusicCut_Free();
}
static void freeEnginesDef()
{
	freeEngines();
}
/**
 *from interface242.cpp:SPK_preprocess_audio
 */
void SPK_preprocess_audio(unsigned idx, bool bUseMusicCut, bool bUseVADCut, 
		char *srcData, unsigned srcLen, char *desData, unsigned *pdesLen)
{
	short *desDataView = (short *)desData;
	int  desViewLen = *pdesLen / sizeof(short);
	short *tmpData = NULL;
	int tmpLen = 0;
	int *pTmpLen = &tmpLen;
	//先sweep music. 再vad. -- from hhj
	if(!bUseVADCut && bUseMusicCut){
		tmpData = desDataView;
		pTmpLen = &desViewLen;
	}
	else if(bUseVADCut && !bUseMusicCut){
		tmpData = (short*)srcData;
		tmpLen = srcLen / sizeof(short);
		pTmpLen = &tmpLen;
	}
	else if(bUseVADCut && bUseMusicCut){
		tmpData = (short *)malloc(srcLen);
		pTmpLen = &tmpLen;
		tmpLen = srcLen / sizeof(short);
		if(tmpData == NULL){
			ERROR_OUTPUT("preprocess_audio Fail to call malloc, sample=%d", srcLen);
			*pdesLen = 0;
			return;
		}
	}
	else {
		*pdesLen = *pdesLen > srcLen ? srcLen : *pdesLen;
		memcpy(desData, srcData, *pdesLen);
		return;
	}

	if(bUseMusicCut) {
		if(!MusicCut(idx, (short*)srcData, srcLen / sizeof(short), tmpData, *pTmpLen)){
			DEBUG_OUTPUT("preprocess_audio MusicCut failed, src samle= %lu\n", srcLen / sizeof(short));
			*pTmpLen = 0;
		}
		else{
			DEBUG_OUTPUT("preprocess_audio MusicCut success, srcSample=%lu; desSample=%lu", srcLen / sizeof(short), *pTmpLen);
		}
	}
	if(bUseVADCut){
		if(!VADBuffer(true, tmpData, *pTmpLen, desDataView, desViewLen)){
			DEBUG_OUTPUT("preprocess_audio ExtractSpeechBuf failed, srcSample=%d\n", *pTmpLen);
			desViewLen=0;
		}
		else{
			DEBUG_OUTPUT("preprocess_audio ExtractSpeechBuf success, srcSample=%d; desSample=%d\n", *pTmpLen, desViewLen);
		}
	}

	if(bUseVADCut && bUseMusicCut){
		free(tmpData);
	}
	*pdesLen = desViewLen * sizeof(short);
}

bool readBinaryFile(const char*wavfile, char **ptrBuf, unsigned *ptrLen)
{
	FILE *fp = fopen(wavfile, "rb");
	if(fp == NULL){
		ERROR_OUTPUT("fail to open file %s", wavfile);
		return false;
	}
	fseek(fp, 0, SEEK_END);
	long int len = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	if(len == -1L)
	{
		ERROR_OUTPUT("fail to call ftell on file %s", wavfile);
		fclose(fp);
		return false;
	}
	*ptrLen = len;
	*ptrBuf = (char*) malloc(sizeof(char) * len);
	if(*ptrBuf == NULL){
		ERROR_OUTPUT("fail to malloc memory, size: %u", *ptrLen);
		fclose(fp);
		return false;
	}
	len = fread(*ptrBuf, 1, *ptrLen, fp);
	if(len != *ptrLen){
		ERROR_OUTPUT("fail to read all of data in file %s, actual: %u; read: %d", wavfile, *ptrLen, len);
		free(*ptrBuf);
		fclose(fp);
		return false;
	}
	return true;
}

bool writeBinaryFile(const char *binFile, char *buf, unsigned len)
{
	FILE *fp = fopen(binFile, "wb");
	if(fp == NULL){
		ERROR_OUTPUT("fail to open file %s", binFile);
		return false;
	}
	size_t wtLen = fwrite(buf, 1, len, fp);
	if(wtLen != len){
		ERROR_OUTPUT("fail to write data to file %s, ready: %u; actual: %u\n", binFile, len, wtLen);
		fclose(fp);
		return false;
	}
	fclose(fp);
	return true;
}
int main(int argc, char *argv[])
{
	char *fromFile = NULL;
	char *toFile = NULL;
	char *srcBuf = NULL;
	unsigned srcLen = 0;
	char *desBuf = NULL;
	unsigned desLen = 0;
	while(true){
		int retc = getopt(argc, argv, "c:f:t:d");
		if(retc == -1) break;
		switch(retc){
			case('c'):
				strncpy(szSpkVADCfg, optarg, MAX_PATH);
				break;
			case('f'):
				fromFile = optarg;
				break;
			case('t'):
				toFile = optarg;
				break;
			case('d'):
				bDebugMode = true;
			default:
				break;
		}

	}
	if(fromFile == NULL || toFile == NULL){
		ERROR_OUTPUT( "usage: program <-d> -f fromFile -t toFile -c cfgFile");
		_Exit(1);
	}
	DEBUG_OUTPUT("cfgFile: %s;\nfromFile: %s;\ntoFile: %s;\ndebugMode: %d", szSpkVADCfg, fromFile, toFile, bDebugMode);
	initEngines();
	atexit(freeEnginesDef);
	if(!readBinaryFile(fromFile, &srcBuf, &srcLen)){
		_Exit(1);
	}
	desBuf = (char*)malloc(sizeof(char) * srcLen);
	if(desBuf == NULL){
		ERROR_OUTPUT("fail to malloc memory. size: %u", srcLen);
		free(srcBuf);
		_Exit(1);
	}
	desLen = srcLen;
	//free(srcBuf);
	//free(desBuf);
	PCM_HEADER pcmhder;
	long unsigned wavhdLen;
	if(!read_wave_header(srcBuf, &pcmhder, &wavhdLen)){
		ERROR_OUTPUT("fail to parse wave head. file %s", fromFile);
		free(srcBuf);
		free(desBuf);
		_Exit(1);
	}

	char *srcDataBuf = srcBuf + wavhdLen;
	unsigned srcDataLen = srcLen - wavhdLen;
	char * desDataBuf = desBuf + sizeof(PCM_HEADER);
	unsigned desDataLen = desLen - sizeof(PCM_HEADER);
	SPK_preprocess_audio(0, false, true, srcDataBuf, srcDataLen, desDataBuf, &desDataLen);
	if(desDataLen == 0){
		free(srcBuf);
		free(desBuf);
		_Exit(0);
	}
	initialize_wave_header((PCM_HEADER *)desBuf, desDataLen);
	writeBinaryFile(toFile, desBuf, desDataLen + sizeof(PCM_HEADER));
	free(srcBuf);
	free(desBuf);
	return 0;
}

