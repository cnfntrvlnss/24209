/*************************************************************************
    > File Name: waveinfo.c
    > Author: ma6174
    > Mail: ma6174@163.com 
    > Created Time: Fri 22 May 2015 03:59:12 AM PDT
 ************************************************************************/

#include<stdio.h>
#include<stdlib.h>
#include <strings.h>
#include<memory.h>
#include <iostream>
#include "waveinfo.h"

void initialize_wave_header(PCM_HEADER *hd, unsigned long len)
{
	memset(hd, 0, sizeof(PCM_HEADER));
	memcpy(hd->riff.fccID, "RIFF", 4);
	hd->riff.dwSize = len + 0x24;
	memcpy(hd->riff.fccType, "WAVE", 4);
	memcpy(hd->fmt.fccID, "fmt ", 4);
	hd->fmt.dwSize = 0x10;
	hd->fmt.wFormatTag = FORMAT_TAG;
	hd->fmt.wChannels = CHANNEL_NUN;
	hd->fmt.dwSamplesPerSec = SAMPLE_RATE;
	hd->fmt.dwAvgBytesPerSec = CHANNEL_NUN*SAMPLE_RATE*BYTES_EACH_SAMPLE;
	hd->fmt.wBlockAlign = BYTES_EACH_SAMPLE;
	hd->fmt.uiBitsPerSample = QUANTIZATION;
	memcpy(hd->dt.fccID, "data", 4);
	hd->dt.dwSize = len;
}

bool read_wave_header(std::istream& is, PCM_HEADER *ptrHd, long *outlen)
{
    long &st = *outlen;
    st = 0;
    is.read(reinterpret_cast<char*>(&ptrHd->riff), sizeof(WAVE_HEADER));
    st += is.gcount();
    is.read(reinterpret_cast<char*>(&ptrHd->fmt), sizeof(WAVE_FORMAT));
    st += is.gcount();
    //skip optional section.
    long optLen = ptrHd->fmt.dwSize - sizeof(WAVE_FORMAT) + 8;
    if(optLen > 0){
        is.seekg(optLen, std::ios_base::cur);
        st += optLen;
        ptrHd->fmt.dwSize -= optLen;
    }
    else if(optLen < 0) return false;
    while(true){
        char szmark[4];
        is.read(szmark, 4);
        st += 4;
        //skip optional section.
        if((strncasecmp(szmark, "fact", 4) == 0) || strncasecmp(szmark, "list", 4) == 0){
            unsigned tmpDataSize;
            is.read(reinterpret_cast<char*>(&tmpDataSize), 4);
            st += 4;
            is.seekg(tmpDataSize, std::ios_base::cur);
            st += tmpDataSize;
        }
        else if(strncmp(szmark, "data", 4) == 0){
            memcpy(reinterpret_cast<char*>(&ptrHd->dt), szmark, 4);
            is.read(reinterpret_cast<char*>(&ptrHd->dt) + 4, sizeof(WAVE_DATA) -4);
            st += is.gcount();
            break;
        }
        else{
            return false;
        }
    }
    if(ptrHd->dt.dwSize + st -8 != ptrHd->riff.dwSize){
        return false;
    }
    return true;
}

bool read_wave_header(char *buf, PCM_HEADER *ptrHd, unsigned long  *ptrSkippedLen)
{
    unsigned st = 0;
    unsigned len = sizeof(WAVE_HEADER);
    memcpy(&ptrHd->riff, buf + st, len);
    st += len;
    len = sizeof(WAVE_FORMAT);
    memcpy(&ptrHd->fmt, buf + st, len);
	//结构体中保存着长度包括了wav_format中的可选部分的长度，要跳过去。
    if (ptrHd->fmt.dwSize > sizeof(WAVE_FORMAT) - 8){
		st += len + ptrHd->fmt.dwSize - sizeof(WAVE_FORMAT) + 8;
		ptrHd->fmt.dwSize = sizeof(WAVE_FORMAT) - 8;
    }
    else if (ptrHd->fmt.dwSize == sizeof(WAVE_FORMAT) - 8){
        st += len;
    }
    else{
        return false;
    }
    //继续跳过 可选部分 。
    if ((strncmp(buf + st, "fact", 4) == 0) || (strncasecmp(buf + st, "list", 4) == 0)){
    unsigned tmpDataSize;
    memcpy(&tmpDataSize, buf + st + 4, 4);
    st += 8 + tmpDataSize;
    }

    if (strncmp(buf + st, "data", 4) == 0){
    memcpy(&ptrHd->dt, buf + st, sizeof(WAVE_DATA));
    st += sizeof(WAVE_DATA);
    if (ptrHd->dt.dwSize + st - 8 != ptrHd->riff.dwSize){
    return false;
    }
    }
    else{
    return false;
    }
    (*ptrSkippedLen) = st;
    return true;
        
}
