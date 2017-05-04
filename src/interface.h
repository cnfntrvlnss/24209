
#pragma once
#include <stdlib.h>
//#include"comm_strcut.h"

struct WAVE 
{
	short* wavbuf;
	int wavlen;
	WAVE()
	{
		wavbuf = NULL;
		wavlen = -1;
	}
};
#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
	StsLicenseExpired = -2,			// 授权过期
	StsNoLicense = -1,				// 没有授权
	StsNoError = 0,					// 成功
	StsNotInit = 1,					// 引擎没有初始化
	StsInitAgain = 2,				// 引擎多次初始化
	StsNoCfgFile = 3,				// 无法打开配置文件
	StsErrCfgItem = 4,				// 配置错误
	StsErrModel = 5,				// 与模型类相关的错误
	StsInvalidInput = 6,			// 无效输入
	StsWavTooShort = 7,				// 语音数据太短
	StsErrAudioSeg = 8,				// 与VAD相关的错误
	StsErrCluster = 9,				// 说话人分段聚类错误
	StsErrExFeat = 10,				// 提取特征失败
	StsNoMemory = 11,				// 分配内存失败
	StsErrOpenFile = 12,			// 无法打开文件
	StsErrWriteFile = 13,			// 写文件失败
	StsErrReadFile = 14,			// 读文件失败
} TITStatus;

	/*TIT_DECLARE*/ TITStatus TIT_SPKID_INIT(const char * cfgfile);
	
	/*TIT_DECLARE*/ TITStatus TIT_SPKID_EXIT();

	/*TIT_DECLARE*/ TITStatus TIT_SPKID_DEL_MDL(void *&mdl);

//	/*TIT_DECLARE*/ TITStatus TIT_SPKID_SAVE_MDL(const void* mdl,const char* modelpath);
//
//	/*TIT_DECLARE*/ TITStatus TIT_SPKID_LOAD_MDL(void *&mdl,const char * modelpath);
//
//	/*TIT_DECLARE*/ TITStatus TIT_SPKID_TRN_SPK_MDL(const WAVE * wavs,const int n,void *&spk);
//
//	/*TIT_DECLARE*/ TITStatus TIT_SPKID_TRN_TEST_MDL(const short * wavbuf,const int wavlen,void *&test);
//	/*TIT_DECLARE*/ TITStatus TIT_SPKID_TRN_TEST_MDL_CLUSTER(const short* wavbuf,const int wavlen,void *&test);
//
//	/*TIT_DECLARE*/ TITStatus TIT_SPKID_TEST_TO_SPK_MDL(const void *const test,void *&spk);
//
//	/*TIT_DECLARE*/ TITStatus TIT_SPKID_VS(const void * test,const void * spk,float &score);
//
//	/*TIT_DECLARE*/ TITStatus TIT_SPKID_SCORE(const void * test,const void ** spks,const int numspk,float *scores);

	/*TIT_DECLARE*/ TITStatus TIT_SPKID_VERIFY_CLUSTER(const short * wavbuf,const int wavlen,const void** spks,const int numspk,float* scores);
                        
        /*TIT_DECLARE*/ TITStatus TIT_SPKID_LOAD_MDL_IVEC(void *&mdl,const char *const modelpath);
        
        /*TIT_DECLARE*/ TITStatus TIT_SPKID_TRN_SPK_MDL_IVEC(const WAVE * wavs,const int n,void *&Ivec);
                        
        /*TIT_DECLARE*/ TITStatus TIT_SPKID_TRN_SPK_MDL_IVEC_VAD(const WAVE * wavs,const int n,void *&Ivec);
                        
        /*TIT_DECLARE*/ TITStatus TIT_SPKID_SAVE_MDL_IVEC(const void *const Ivec,const char *const modelpath);  
#ifdef __cplusplus
}
#endif
