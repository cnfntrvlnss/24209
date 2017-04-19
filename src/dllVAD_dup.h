/*************************************************************************
    > File Name: dllVAD_dup.h
    > Author: zhengshurui
    > Mail:  zhengshurui@thinkit.cn
    > Created Time: Fri 07 Oct 2016 06:13:03 PM PDT
 ************************************************************************/
#ifndef DLLVAD_DUP__H
#define DLLVAD_DUP__H

#include "dllVAD.h"

int openOneVAD(char *cfgFile);
void closeOneVAD(int hdl);

inline bool cutVAD(int hdl, short* inputBuf, unsigned inputLen, short* &outputBuf, unsigned &outputLen)
{
    int oLen;
    ExtractSpeechBuf(hdl, inputBuf, inputLen, outputBuf, oLen);
    outputLen = oLen;
    return true;
}
#endif
