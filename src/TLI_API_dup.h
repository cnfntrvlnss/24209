/*************************************************************************
    > File Name: TLI_API_dup.h
    > Author: zhengshurui
    > Mail:  zhengshurui@thinkit.cn
    > Created Time: Sat 08 Oct 2016 03:17:57 AM PDT
 ************************************************************************/

#ifndef TLI__API__DUP_H
#define TLI__API__DUP_H


bool initLID(unsigned thrdNum);
int openTLI_dup();
void closeTLI_dup(int hdl);

void scoreTLI_dup(int hdl, short *pcmBuf, int pcmLen, int &resID, float &resScore);

#endif
