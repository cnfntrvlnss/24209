/*************************************************************************
    > File Name: MusicDetect_dup.h
    > Author: zhengshurui
    > Mail:  zhengshurui@thinkit.cn
    > Created Time: Sat 08 Oct 2016 02:43:23 AM PDT
 ************************************************************************/
#ifndef MUSICDETECT__DUP_H
#define MUSICDETECT__DUP_H

typedef struct MscCutInst{}* MscCutHandle;

MscCutHandle openMusicCut(const char *cfgFile);
void finishOpenMusicCut();
bool cutMusic(MscCutHandle hdl, short* src, unsigned iLen, short* &dst, unsigned &oLen);
void closeMusicCut(MscCutHandle hdl);

#endif


