/*************************************************************************
	> File Name: /home/zheng/Desktop/242_09/src/sni_kw_ex.h
	> Author: 
	> Mail: 
	> Created Time: Thu 04 May 2017 10:11:53 PM EDT
 ************************************************************************/

#ifndef SNI_KW_EX_H
#define SNI_KW_EX_H
#include <stdlib.h>

void sni_init(unsigned thrdnum);
bool sni_close(int hdl);
bool sni_open(int &hdl);
bool isAudioSynthetic(int hdl, short *pcmdata, unsigned pcmlen, int &maxscore);

bool tkw_init(int sessnum, const char *kwSysDir = NULL);
bool tkw_open(int &sid);
bool tkw_close(int sid);
bool tkw_recognize(int sid, short *pcmdata, unsigned pcmlen, const char* wavefile);
bool tkw_recognize(int sid, short *pcmdata, unsigned pcmlen, const char *wavfile, char* &output, unsigned len, unsigned &cfgId);
#endif
