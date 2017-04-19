/* 
 * File: hhj.h
 * Author: huanghoujun
 *
 * Created on 2013��8��30��, ����5:21
 */

#ifndef __HHJ__H__
#define	__HHJ__H__

#include "../include/interface242/interface242.h"

#ifdef HAVE_SPKREC

#define MAX_SPKMODEL_NUM        500
extern void* g_pSpeakerModel[MAX_SPKMODEL_NUM];
extern int g_TSI_nSpkID[MAX_SPKMODEL_NUM];
extern int g_Spk_HarmLevel[MAX_SPKMODEL_NUM];
extern char g_Spk_Flag[MAX_SPKMODEL_NUM];
extern float SpkThreshold[MAX_SPKMODEL_NUM + 1];
extern int g_TSI_nSpkMdlNum;
extern pthread_rwlock_t g_spklistupdating;
bool spkRegProcess(short *waveData, int smpNum, RESULT_STRUCT &result, int thread_id);
#endif

extern int g_TEMPLATENUM;
extern int *pnAllTemplateIDs;    
extern int g_SecondVAD;
extern int g_bUseDetector;
extern int g_MaxLIDLen;
extern int g_MinLIDLen;

bool LIDScore(short* wavdata,int len,int &nmax,float &s,int threadid);

//bool if_directory_exists(const char *dir, bool bForce = false);
//char* GetLocalIP();

#endif	/* HHJ_H */

