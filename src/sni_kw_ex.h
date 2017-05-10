/*************************************************************************
	> File Name: /home/zheng/Desktop/242_09/src/sni_kw_ex.h
	> Author: 
	> Mail: 
	> Created Time: Thu 04 May 2017 10:11:53 PM EDT
 ************************************************************************/

#ifndef SNI_KW_EX_H
#define SNI_KW_EX_H

void sni_init(unsigned thrdnum);
bool sni_close(int hdl);
bool sni_open(int &hdl);
bool isAudioSynthetic(int hdl, short *pcmdata, unsigned pcmlen, int &maxscore);

bool tbnr_init(int sessnum);
bool tbnr_start(int &sid);
bool tbnr_stop(int sid);
bool tbnr_recognize(int sid, short *pcmdata, unsigned pcmlen, const char* outfile);
#endif
