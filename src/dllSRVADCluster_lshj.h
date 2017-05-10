/********************************************************************
	created:	2013/05/22
	filename: 	dllSRVADCluster.h
	author:		wanyulong
	
	purpose:	VAD(Voice Activity Detection) 
	            Clustering for Speaker Recognition.
				Multiple Threads supported.
*********************************************************************/
#ifndef DLLSRVADCLUSTER_H
#define DLLSRVADCLUSTER_H

#include <vector>
using namespace std;

#ifdef WIN32
#ifdef DLLSRVADCLUSTER_EXPORTS
#define DLLSRVADCLUSTER_API __declspec(dllexport)
#else
#define DLLSRVADCLUSTER_API __declspec(dllimport)
#endif
#else
#define DLLSRVADCLUSTER_API
#endif


struct VADCfg
{
    int iOutTimeSec; // Output buffer time period.
    int iMinProcTimeSec; // Min file time to process.
    int iMaxProcTimeSec; // Max file time to process.
    int iDetectTimeSec; // Max file time to detect music and ring.
    int iMaxMidFrmNum;

    bool bDetectRing; // Whether use the Ring Detector.
    bool bDetectSong; // Whether use the Song Detector.
    bool bDetectMusic; // Whether use the Music Detector.

    VADCfg():iOutTimeSec(180),iMinProcTimeSec(3),iMaxProcTimeSec(1800),iDetectTimeSec(60),iMaxMidFrmNum(20),bDetectMusic(true),bDetectSong(true),bDetectRing(true){};
};

////////////////////////////////////2016,12,26, buf >> struct
typedef struct SEGMENTS_INFO
{
	int state;			//////语音段的状态，0为静音，1为语音，2为其它
	int start_time;		//////语音段的开始采样点
	int end_time;		//////语音段的结束采样点
}SegmentsInfo,*pSegmentsInfo;
typedef struct VAD_SEG_INFO
{
	int num_segments;	//////语音段的总数
	pSegmentsInfo segs_info;

	VAD_SEG_INFO():num_segments(0),segs_info(NULL){};
}VadSegInfo,*pVadSegInfo;
typedef struct REPEAT_INFO{
	bool detect_repeat;			/////是否检测重复的字词
	int winsize_repeat;			/////重复字词的窗长（ms）
	float energy_repeat_ratio;	////两帧重复字词能量的比值（0~1）
	float zcr_repeat_ratio;		////两帧重复字词过零率的比值（0~1）
	float words_repeat_ratio;	///重复字词在语音段中的占比（0~1）
	REPEAT_INFO():detect_repeat(true),winsize_repeat(200),energy_repeat_ratio(0.9),zcr_repeat_ratio(0.9),words_repeat_ratio(0.9){};
}RepeatInfo,*pRepeatInfo;
//////////////////////////////////////////////////////////////
// Cluster Config Class, added by wanyulong 2013/08/20
struct ClusterCfg
{
    int iDstClusterNum; // number of destination cluster.
    int iSingleOutTimeSec; // max out buffer time period for each cluster. unit: second
    int iSingleMinTimeSec; // min time period of one cluster. unit: second

    ClusterCfg():iDstClusterNum(2),iSingleOutTimeSec(180),iSingleMinTimeSec(15){};
};

//extern "C"
//{
	/***************************函数说明******************************************
	*F函数名称			：Get_version
	*F函数功能			：获得引擎版本号
	*F函数类型			：DLLSRVADCLUSTER_API int
	*F函数参数			：char* version	输出 版本信息
	*F函数返回值		：0		成功
	*F					：-1	失败
	*****************************************************************************/
	DLLSRVADCLUSTER_API int Get_version(char* version);

	/***************************函数说明******************************************
	*F函数名称			：InitVADCluster_File
	*F函数功能			：初始化引擎
	*F函数类型			：DLLSRVADCLUSTER_API bool
	*F函数参数			：无
	*F函数返回值		：true		成功
	*F					：false		失败
	*****************************************************************************/
	DLLSRVADCLUSTER_API bool InitVADCluster_File();

	/***************************函数说明******************************************
	*F函数名称			：InitVADCluster
	*F函数功能			：初始化引擎
	*F函数类型			：DLLSRVADCLUSTER_API bool
	*F函数参数			：const VADCfg& rsVADCfg			输出 有效音配置信息
	*F函数参数			：const ClusterCfg& rsClusterCfg	输出 聚类配置信息
	*F函数返回值		：true		成功
	*F					：false		失败
	*****************************************************************************/
    DLLSRVADCLUSTER_API bool InitVADCluster(const VADCfg& rsVADCfg, const ClusterCfg& rsClusterCfg);

	/***************************函数说明******************************************
	*F函数名称			：VADBuffer
	*F函数功能			：有效音检测
	*F函数类型			：DLLSRVADCLUSTER_API bool
	*F函数参数			：const bool bAllOut			输入 有效音是否全部输出
	*F函数参数			：const short* psPCMBuffer		输入 语音buf（ 8k,16bit, linear PCM ）
	*F函数参数			：const int iSampleNum			输入 语音buf长度（语音采样点个数）
	*F函数参数			：short* psPCMBufferVAD			输出 vad后的buf（ 8k,16bit, linear PCM ）
	*F函数参数			：int& riSampleNumVAD			输出 vad后的buf长度（语音采样点个数）
	*F函数参数			：pVadSegInfo *vad_segments_info	输出 语音段详细信息
	*F函数参数			：bool bUseDetector				输入 是否进行彩振铃检测
	*F函数返回值		：true		成功
	*F					：false		失败
	*****************************************************************************/
	DLLSRVADCLUSTER_API bool VADBuffer(const bool bAllOut, const short* psPCMBuffer, const int iSampleNum, short* psPCMBufferVAD, int& riSampleNumVAD, bool bUseDetector=true);

	DLLSRVADCLUSTER_API bool VADBuffer(const bool bAllOut, const short* psPCMBuffer, const int iSampleNum, pVadSegInfo *vad_segments_info, bool bUseDetector=true);

	/***************************函数说明******************************************
	*F函数名称			：VADRepeatDetect
	*F函数功能			：重复字词检测
	*F函数类型			：DLLSRVADCLUSTER_API int
	*F函数参数			：const short*			输入 语音buf（ 8k,16bit, linear PCM ）
	*F函数参数			：const int buf_num		输入 语音buf长度（语音采样点个数）
	*F函数参数			：pRepeatInfo repeat_info			输入 重复字词的信息
	*F函数返回值		：0			正常语音
	*F					：-1		输入参数错误
	*F					：-2		不使用此接口
	*F					：-3		语音太短（不足2个重复帧）
	*F					：-4		重复字词语音
	*****************************************************************************/
	DLLSRVADCLUSTER_API int VADRepeatDetect(const short* buf, const int buf_num, pRepeatInfo repeat_info);

	/***************************函数说明******************************************
	*F函数名称			：VADClusterBuffer
	*F函数功能			：语音聚类
	*F函数类型			：DLLSRVADCLUSTER_API bool
	*F函数参数			：const short* psInWavData		输入 语音buf（ 8k,16bit, linear PCM ）
	*F函数参数			：const int iInSampleNum		输入 语音buf长度（语音采样点个数）
	*F函数参数			：short* psOutWavDataC1			输出 聚类1的buf
	*F函数参数			：int& riOutSampleNumC1			输出 聚类1的buf长度
	*F函数参数			：short* psOutWavDataC2			输出 聚类2的buf
	*F函数参数			：int& riOutSampleNumC2			输出 聚类2的buf长度
	*F函数返回值		：true		成功
	*F					：false		失败
	*****************************************************************************/
	DLLSRVADCLUSTER_API bool VADClusterBuffer(const short* psInWavData, const int iInSampleNum, short* psOutWavDataC1, int& riOutSampleNumC1, short* psOutWavDataC2, int& riOutSampleNumC2);

	/***************************函数说明******************************************
	*F函数名称			：FreeVADCluster
	*F函数功能			：释放引擎
	*F函数类型			：DLLSRVADCLUSTER_API bool
	*F函数参数			：无
	*F函数返回值		：true		成功
	*F					：false		失败
	*****************************************************************************/
	DLLSRVADCLUSTER_API bool FreeVADCluster();
//};

#endif

