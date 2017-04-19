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

//////////////////////////////////////////////////////////////
// Cluster Config Class, added by wanyulong 2013/08/20
struct ClusterCfg
{
    int iDstClusterNum; // number of destination cluster.
    int iSingleOutTimeSec; // max out buffer time period for each cluster. unit: second
    int iSingleMinTimeSec; // min time period of one cluster. unit: second

    ClusterCfg():iDstClusterNum(2),iSingleOutTimeSec(180),iSingleMinTimeSec(15){};
};

extern "C"
{
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
	*F函数参数			：short* psPCMBufferVAD			输出 有效音buf
	*F函数参数			：int& riSampleNumVAD			输出 有效音buf长度
	*F函数参数			：bool bUseDetector				输入 是否进行彩振铃检测
	*F函数返回值		：true		成功
	*F					：false		失败
	*****************************************************************************/
	DLLSRVADCLUSTER_API bool VADBuffer(const bool bAllOut, const short* psPCMBuffer, const int iSampleNum, short* psPCMBufferVAD, int& riSampleNumVAD, bool bUseDetector=true);

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
};

#endif

