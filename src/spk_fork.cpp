/*************************************************************************
	> File Name: spk_fork.cpp
	> Author: 
	> Mail: 
	> Created Time: Sun 27 Nov 2016 03:57:50 AM EST
 ************************************************************************/

//#include "spkEngine.h"
#include "TIT_SPKID_OFFL_API.h"
#include "dllSRVADCluster.h"
#include "MusicDetect.h"
#include <cstdlib>
#include<iostream>
#include <cstdio>
#include <cstring>
using namespace std;

#define OUTPUT_FORK(x) fprintf(stderr, #x " in %s.\n", __FILE__)
/*
bool InitVADCluster(const char* CfgPath)
{
    OUTPUT_FORK(InitVADCluster);
    return true;
}
 bool VADBuffer(const bool bAllOut, const short* psPCMBuffer, const int iSampleNum, short* psPCMBufferVAD, int& riSampleNumVAD, bool bUseDetector)
{
    OUTPUT_FORK(VADBuffer);
    int wlen = iSampleNum < riSampleNumVAD ? iSampleNum : riSampleNumVAD;
    memcpy(psPCMBufferVAD, psPCMBuffer, wlen * sizeof(short));
    riSampleNumVAD = wlen;
    return true;
}
bool VADClusterBuffer(const short* psInWavData, const int iInSampleNum, short* psOutWavDataC1, int& riOutSampleNumC1, short* psOutWavDataC2, int& riOutSampleNumC2)
{
    OUTPUT_FORK(VADClusterBuffer);
    return false;
}
bool FreeVADCluster()
{
    OUTPUT_FORK(FreeVADCluster);
    return true;
}
*/

#ifndef LIDFORK
bool MusicCut_Initial(char* CfgFile,int ThreadNum)
{
    OUTPUT_FORK(Musiccut_initial);
    return true;
}
bool MusicCut(int ThreadIndex,short* src,int iLen,short* &dst,int &oLen)
{
    int wlen = iLen < oLen ? iLen : oLen;
    memcpy(dst, src, wlen * sizeof(short));
    oLen = wlen;
    return true;
}
bool MusicCut_Free()
{
    OUTPUT_FORK(musiccut_free);
    return true;
}
#endif

#ifdef SPEAKID_2010_4_17

// 初始化
TIT_DECLDIR TIT_RET_CODE TIT_SPKID_Init(
    char    *p_cConfigFile,                // 配置文件
    char    *p_cCurModelPath,              // 模型路径
    int     &p_nFeatLen,                   //特征大小
    int     &p_nModelLen,                  // 模型大小
    int		&p_nIndexlen)                  // 索引大小
{
    OUTPUT_FORK(TIT_SPKID_Init);
    p_nFeatLen = p_nModelLen = p_nIndexlen = 1;
    return TIT_SPKID_SUCCESS;
}

TIT_DECLDIR TIT_RET_CODE TIT_Feat_To_Model(
    void    *p_pFeatModel,		        //语音特征
    void	*p_pSpeakerModel)			//语音模型
{
    OUTPUT_FORK(TIT_Feat_To_Model);
    //p_pSpeakerModel[0] = '\0';
    return TIT_SPKID_SUCCESS;
}

// 结束
TIT_DECLDIR TIT_RET_CODE TIT_SPKID_Exit()
{
    OUTPUT_FORK(TIT_SPKID_Exit);
    return TIT_SPKID_SUCCESS;
}

// Buf接口的多个说话人识别接口, 切静音+说话人聚类
TIT_DECLDIR TIT_RET_CODE TIT_SCR_Buf_AddCfd_CutSil_Cluster(
    short   *p_sDat,                      // 音频数据8K-16Bit
    int     p_nDatLen,				      // 音频数据长度（采样点数目）
    void    **p_pSpeakerModel,		      // 待识别的模型数组
    float   *&p_pfSpkScore,			      // 模型得分
    int     p_nModelNum,			      // 当前加载的模型数目
    int     &p_nMatchModelIndex,          // 返回识别出的模型index，若小于零，则表示拒识
    char    *wavfile)             // 调试用, 置为 NULL
{
    OUTPUT_FORK(TIT_SCR_Buf_AddCfg_CutSil_Cluster);
    if(p_nModelNum > 0){
        p_pfSpkScore[0] = 0.0;
        p_nMatchModelIndex = 0;
    }
    return TIT_SPKID_SUCCESS;
}

#else
TITStatus TIT_SPKID_INIT(const char * cfgfile)
{
    OUTPUT_FORK(tit_spkid_init);
    return StsNoError;
}

TITStatus TIT_SPKID_EXIT()
{
    OUTPUT_FORK(tit_spkid_exit);
    return StsNoError;
}

TITStatus TIT_SPKID_DEL_MDL(void *&mdl)
{
    OUTPUT_FORK(tit_spkid_del_mdl);
    free(mdl);
    return StsNoError;
}

TITStatus TIT_SPKID_VERIFY_CLUSTER(const short * wavbuf,const int wavlen,const void** spks,const int numspk,float* scores)
{
    OUTPUT_FORK(tit_spkid_verify_cluster);
    *scores = 100;
    return StsNotInit;
}
            
TITStatus TIT_SPKID_LOAD_MDL_IVEC(void *&mdl,const char *const modelpath)
{
    OUTPUT_FORK(tit_spkid_load_mdl_ivec);
    mdl = malloc(1);
    return StsNoError;
}
TITStatus TIT_SPKID_SAVE_MDL_IVEC(const void *const Ivec,const char *const modelpath)
{
    OUTPUT_FORK(tit_spkid_saved_mdl_ivec);
    return StsNoError;
}
#endif
